/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowMusicBase.h"

#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "music/MusicDbUrl.h"
#include "music/MusicLibraryQueue.h"
#include "music/MusicUtils.h"
#include "music/dialogs/GUIDialogInfoProviderSettings.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif
#include "Autorun.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "GUIPassword.h"
#include "PartyModeManager.h"
#include "URL.h"
#include "addons/gui/GUIDialogAddonInfo.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/tags/MusicInfoTag.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "view/GUIViewState.h"

#include <algorithm>

using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;
using namespace MUSIC_GRABBER;
using namespace MUSIC_INFO;
using namespace KODI::MESSAGING;
using KODI::MESSAGING::HELPERS::DialogResponse;

using namespace std::chrono_literals;

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_BTNPLAYLISTS    7
#define CONTROL_BTNSCAN         9
#define CONTROL_BTNRIP          11

CGUIWindowMusicBase::CGUIWindowMusicBase(int id, const std::string &xmlFile)
    : CGUIMediaWindow(id, xmlFile.c_str())
{
  m_dlgProgress = NULL;
  m_thumbLoader.SetObserver(this);
}

CGUIWindowMusicBase::~CGUIWindowMusicBase () = default;

bool CGUIWindowMusicBase::OnBack(int actionID)
{
  if (!CMusicLibraryQueue::GetInstance().IsScanningLibrary())
  {
    CUtil::RemoveTempFiles();
  }
  return CGUIMediaWindow::OnBack(actionID);
}

/*!
 \brief Handle messages on window.
 \param message GUI Message that can be reacted on.
 \return if a message can't be processed, return \e false

 On these messages this class reacts.\n
 When retrieving...
  - #GUI_MSG_WINDOW_DEINIT\n
   ...the last focused control is saved to m_iLastControl.
  - #GUI_MSG_WINDOW_INIT\n
   ...the musicdatabase is opend and the music extensions and shares are set.
   The last focused control is set.
  - #GUI_MSG_CLICKED\n
   ... the base class reacts on the following controls:\n
    Buttons:\n
    - #CONTROL_BTNVIEWASICONS - switch between list, thumb and with large items
    - #CONTROL_BTNSEARCH - Search for items\n
    Other Controls:
    - The container controls\n
     Have the following actions in message them clicking on them.
     - #ACTION_QUEUE_ITEM - add selected item to end of playlist
     - #ACTION_QUEUE_ITEM_NEXT - add selected item to next pos in playlist
     - #ACTION_SHOW_INFO - retrieve album info from the internet
     - #ACTION_SELECT_ITEM - Item has been selected. Overwrite OnClick() to react on it
 */
bool CGUIWindowMusicBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
      m_musicdatabase.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_dlgProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);

      m_musicdatabase.Open();

      if (!CGUIMediaWindow::OnMessage(message))
        return false;

      return true;
    }
    break;
  case GUI_MSG_DIRECTORY_SCANNED:
    {
      CFileItem directory(message.GetStringParam(), true);

      // Only update thumb on a local drive
      if (directory.IsHD())
      {
        std::string strParent;
        URIUtils::GetParentPath(directory.GetPath(), strParent);
        if (directory.GetPath() == m_vecItems->GetPath() || strParent == m_vecItems->GetPath())
          Refresh();
      }
    }
    break;

  // update the display
  case GUI_MSG_SCAN_FINISHED:
  case GUI_MSG_REFRESH_THUMBS: // Never called as is secondary msg sent as GUI_MSG_NOTIFY_ALL
    Refresh();
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNRIP)
      {
        OnRipCD();
      }
      else if (iControl == CONTROL_BTNPLAYLISTS)
      {
        if (!m_vecItems->IsPath("special://musicplaylists/"))
          Update("special://musicplaylists/");
      }
      else if (iControl == CONTROL_BTNSCAN)
      {
        OnScan(-1);
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
        }
        else if (iAction == ACTION_QUEUE_ITEM_NEXT)
        {
          OnQueueItem(iItem, true);
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          OnItemInfo(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          // must be at the playlists directory
          if (m_vecItems->IsPath("special://musicplaylists/"))
            OnDeleteItem(iItem);

          else
            return false;
        }
        // use play button to add folders of items to temp playlist
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          const auto& components = CServiceBroker::GetAppComponents();
          const auto appPlayer = components.GetComponent<CApplicationPlayer>();
          // if playback is paused or playback speed != 1, return
          if (appPlayer->IsPlayingAudio())
          {
            if (appPlayer->IsPausedPlayback())
              return false;
            if (appPlayer->GetPlaySpeed() != 1)
              return false;
          }

          // not playing audio, or playback speed == 1
          PlayItem(iItem);

          return true;
        }
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1()==GUI_MSG_REMOVED_MEDIA)
        CUtil::DeleteDirectoryCache("r-");
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowMusicBase::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SHOW_PLAYLIST)
  {
    if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC ||
        CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_MUSIC).size() > 0)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_PLAYLIST);
      return true;
    }
  }

  if (action.GetID() == ACTION_SCAN_ITEM)
  {
    int item = m_viewControl.GetSelectedItem();
    if (item > -1 && m_vecItems->Get(item)->m_bIsFolder)
      OnScan(item);

    return true;
  }

  return CGUIMediaWindow::OnAction(action);
}

void CGUIWindowMusicBase::OnItemInfoAll(const std::string& strPath, bool refresh)
{
  if (StringUtils::EqualsNoCase(m_vecItems->GetContent(), "albums"))
  {
    if (CMusicLibraryQueue::GetInstance().IsScanningLibrary())
      return;

    CMusicLibraryQueue::GetInstance().StartAlbumScan(strPath, refresh);
  }
  else if (StringUtils::EqualsNoCase(m_vecItems->GetContent(), "artists"))
  {
    if (CMusicLibraryQueue::GetInstance().IsScanningLibrary())
      return;

    CMusicLibraryQueue::GetInstance().StartArtistScan(strPath, refresh);
  }
}

void CGUIWindowMusicBase::OnItemInfo(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size() )
    return;

  CFileItemPtr item = m_vecItems->Get(iItem);

  // Match visibility test of CMusicInfo::IsVisible
  if (item->IsVideoDb() && item->HasVideoInfoTag() &&
      (item->HasProperty("artist_musicid") || item->HasProperty("album_musicid")))
  {
    // Music video artist or album (navigation by music > music video > artist))
    CGUIDialogMusicInfo::ShowFor(item.get());
    return;
  }

  if (item->IsVideo() && item->HasVideoInfoTag() &&
      item->GetVideoInfoTag()->m_type == MediaTypeMusicVideo)
  { // Music video on a mixed current playlist or navigation by music > music video > artist > video
    CGUIDialogVideoInfo::ShowFor(*item);
    return;
  }

  if (!m_vecItems->IsPlugin() && (item->IsPlugin() || item->IsScript()))
  {
    CGUIDialogAddonInfo::ShowForItem(item);
    return;
  }

  // Match visibility test of CMusicInfo::IsVisible
  if (item->HasMusicInfoTag() && (item->GetMusicInfoTag()->GetType() == MediaTypeSong ||
    item->GetMusicInfoTag()->GetType() == MediaTypeAlbum ||
    item->GetMusicInfoTag()->GetType() == MediaTypeArtist))
    CGUIDialogMusicInfo::ShowFor(item.get());
}

void CGUIWindowMusicBase::RefreshContent(const std::string& strContent)
{
  if ( CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_MUSIC_NAV &&
    m_vecItems->GetContent() == strContent &&
    m_vecItems->GetSortMethod() == SortByUserRating)
    // When music library window is active and showing songs or albums sorted
    // by userrating refresh the list to resort items and show new userrating
    Refresh(true);
}

/// \brief Retrieve tag information for \e m_vecItems
void CGUIWindowMusicBase::RetrieveMusicInfo()
{
  auto start = std::chrono::steady_clock::now();

  OnRetrieveMusicInfo(*m_vecItems);

  //! @todo Scan for multitrack items here...
  std::vector<std::string> itemsForRemove;
  CFileItemList itemsForAdd;
  for (int i = 0; i < m_vecItems->Size(); ++i)
  {
    CFileItemPtr pItem = (*m_vecItems)[i];
    if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsPicture() || pItem->IsLyrics() || pItem->IsVideo())
      continue;

    CMusicInfoTag& tag = *pItem->GetMusicInfoTag();
    if (tag.Loaded() && !tag.GetCueSheet().empty())
      pItem->LoadEmbeddedCue();

    if (pItem->HasCueDocument()
      && pItem->LoadTracksFromCueDocument(itemsForAdd))
    {
      itemsForRemove.push_back(pItem->GetPath());
    }
  }
  for (size_t i = 0; i < itemsForRemove.size(); ++i)
  {
    for (int j = 0; j < m_vecItems->Size(); ++j)
    {
      if ((*m_vecItems)[j]->GetPath() == itemsForRemove[i])
      {
        m_vecItems->Remove(j);
        break;
      }
    }
  }
  m_vecItems->Append(itemsForAdd);

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  CLog::Log(LOGDEBUG, "RetrieveMusicInfo() took {} ms", duration.count());
}

/// \brief Add selected list/thumb control item to playlist and start playing
/// \param iItem Selected Item in list/thumb control
void CGUIWindowMusicBase::OnQueueItem(int iItem, bool first)
{
  // don't re-queue items from playlist window
  if (iItem < 0 || iItem >= m_vecItems->Size() || GetID() == WINDOW_MUSIC_PLAYLIST)
    return;

  // add item 2 playlist
  const auto item = m_vecItems->Get(iItem);

  if (item->IsRAR() || item->IsZIP())
    return;

  MUSIC_UTILS::QueueItem(item, first ? MUSIC_UTILS::QueuePosition::POSITION_BEGIN
                                     : MUSIC_UTILS::QueuePosition::POSITION_END);

  // select next item
  m_viewControl.SetSelectedItem(iItem + 1);
}

void CGUIWindowMusicBase::UpdateButtons()
{
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTNRIP, CServiceBroker::GetMediaManager().IsAudio());

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTNSCAN,
                              !(m_vecItems->IsVirtualDirectoryRoot() ||
                                m_vecItems->IsMusicDb()));

  if (CMusicLibraryQueue::GetInstance().IsScanningLibrary())
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 14056); // Stop Scan
  else
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 102); // Scan

  CGUIMediaWindow::UpdateButtons();
}

void CGUIWindowMusicBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  if (item)
  {
    const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    // Check for the partymode playlist item.
    // When "PartyMode.xsp" not exist, only context menu button is edit
    if (item->IsSmartPlayList() &&
        (item->GetPath() == profileManager->GetUserDataItem("PartyMode.xsp")) &&
        !CFileUtils::Exists(item->GetPath()))
    {
      buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
      return;
    }

    if (!item->IsParentFolder())
    {
      //! @todo get rid of IsAddonsPath and IsScript check. CanQueue should be enough!
      if (item->CanQueue() && !item->IsAddonsPath() && !item->IsScript())
      {
        if (item->IsSmartPlayList())
          buttons.Add(CONTEXT_BUTTON_PLAY_PARTYMODE, 15216); // Play in Partymode

        if (item->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
          buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
        else if (item->IsPlayList() || m_vecItems->IsPlayList())
          buttons.Add(CONTEXT_BUTTON_EDIT, 586);
      }
#ifdef HAS_OPTICAL_DRIVE
      // enable Rip CD Audio or Track button if we have an audio disc
      if (CServiceBroker::GetMediaManager().IsDiscInDrive() && m_vecItems->IsCDDA())
      {
        // those cds can also include Audio Tracks: CDExtra and MixedMode!
        MEDIA_DETECT::CCdInfo* pCdInfo = CServiceBroker::GetMediaManager().GetCdInfo();
        if (pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1))
          buttons.Add(CONTEXT_BUTTON_RIP_TRACK, 610);
      }
#endif
    }

    // enable CDDB lookup if the current dir is CDDA
    if (CServiceBroker::GetMediaManager().IsDiscInDrive() && m_vecItems->IsCDDA() &&
        (profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      buttons.Add(CONTEXT_BUTTON_CDDB, 16002);
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

void CGUIWindowMusicBase::GetNonContextButtons(CContextButtons &buttons)
{
}

bool CGUIWindowMusicBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  if (CGUIDialogContextMenu::OnContextButton("music", item, button))
  {
    if (button == CONTEXT_BUTTON_REMOVE_SOURCE)
      OnRemoveSource(itemNumber);

    Update(m_vecItems->GetPath());
    return true;
  }

  switch (button)
  {
    case CONTEXT_BUTTON_INFO:
      OnItemInfo(itemNumber);
      return true;

    case CONTEXT_BUTTON_EDIT:
    {
      std::string playlist = item->IsPlayList() ? item->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_PLAYLIST_EDITOR, playlist);
      // need to update
      m_vecItems->RemoveDiscCache(GetID());
      return true;
    }

  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      std::string playlist = item->IsSmartPlayList() ? item->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist, "music"))
        Refresh(true); // need to update
      return true;
    }

  case CONTEXT_BUTTON_PLAY_PARTYMODE:
    g_partyModeManager.Enable(PARTYMODECONTEXT_MUSIC, item->GetPath());
    return true;

  case CONTEXT_BUTTON_RIP_CD:
    OnRipCD();
    return true;

#ifdef HAS_CDDA_RIPPER
  case CONTEXT_BUTTON_CANCEL_RIP_CD:
    KODI::CDRIP::CCDDARipper::GetInstance().CancelJobs();
    return true;
#endif

  case CONTEXT_BUTTON_RIP_TRACK:
    OnRipTrack(itemNumber);
    return true;

  case CONTEXT_BUTTON_SCAN:
    // Check if scanning already and inform user
    if (CMusicLibraryQueue::GetInstance().IsScanningLibrary())
      HELPERS::ShowOKDialogText(CVariant{ 189 }, CVariant{ 14057 });
    else
      OnScan(itemNumber, true);
    return true;

  case CONTEXT_BUTTON_CDDB:
    if (m_musicdatabase.LookupCDDBInfo(true))
      Refresh();
    return true;

  default:
    break;
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowMusicBase::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("music");
}

void CGUIWindowMusicBase::OnRipCD()
{
  if (CServiceBroker::GetMediaManager().IsAudio())
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
#ifdef HAS_CDDA_RIPPER
      KODI::CDRIP::CCDDARipper::GetInstance().RipCD();
#endif
    }
    else
      HELPERS::ShowOKDialogText(CVariant{257}, CVariant{20099});
  }
}

void CGUIWindowMusicBase::OnRipTrack(int iItem)
{
  if (CServiceBroker::GetMediaManager().IsAudio())
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
#ifdef HAS_CDDA_RIPPER
      CFileItemPtr item = m_vecItems->Get(iItem);
      KODI::CDRIP::CCDDARipper::GetInstance().RipTrack(item.get());
#endif
    }
    else
      HELPERS::ShowOKDialogText(CVariant{257}, CVariant{20099});
  }
}

void CGUIWindowMusicBase::PlayItem(int iItem)
{
  // restrictions should be placed in the appropriate window code
  // only call the base code if the item passes since this clears
  // the current playlist

  const CFileItemPtr pItem = m_vecItems->Get(iItem);
#ifdef HAS_OPTICAL_DRIVE
  if (pItem->IsDVD())
  {
    MEDIA_DETECT::CAutorun::PlayDiscAskResume(pItem->GetPath());
    return;
  }
#endif

  // Check for the partymode playlist item, do nothing when "PartyMode.xsp" not exist
  if (pItem->IsSmartPlayList())
  {
    const std::shared_ptr<CProfileManager> profileManager =
        CServiceBroker::GetSettingsComponent()->GetProfileManager();
    if ((pItem->GetPath() == profileManager->GetUserDataItem("PartyMode.xsp")) &&
        !CFileUtils::Exists(pItem->GetPath()))
      return;
  }

  // if its a folder, build a playlist
  if (pItem->m_bIsFolder && !pItem->IsPlugin())
  {
    // make a copy so that we can alter the queue state
    CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item->CanQueue())
      item->SetCanQueue(true);

    // skip ".."
    if (item->IsParentFolder())
      return;

    CFileItemList queuedItems;
    MUSIC_UTILS::GetItemsForPlayList(item, queuedItems);
    if (g_partyModeManager.IsEnabled())
    {
      g_partyModeManager.AddUserSongs(queuedItems, true);
      return;
    }

    /*
    std::string strPlayListDirectory = m_vecItems->GetPath();
    URIUtils::RemoveSlashAtEnd(strPlayListDirectory);
    */

    CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::TYPE_MUSIC);
    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::TYPE_MUSIC, queuedItems);
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::TYPE_MUSIC);

    // play!
    CServiceBroker::GetPlaylistPlayer().Play();
  }
  else if (pItem->IsPlayList())
  {
    // load the playlist the old way
    LoadPlayList(pItem->GetPath());
  }
  else
  {
    // just a single item, play it
    //! @todo Add music-specific code for single playback of an item here (See OnClick in MediaWindow, and OnPlayMedia below)
    OnClick(iItem);
  }
}

void CGUIWindowMusicBase::LoadPlayList(const std::string& strPlayList)
{
  // if partymode is active, we disable it
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  std::unique_ptr<PLAYLIST::CPlayList> pPlayList(PLAYLIST::CPlayListFactory::Create(strPlayList));
  if (pPlayList)
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      HELPERS::ShowOKDialogText(CVariant{6}, CVariant{477});
      return; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST::TYPE_MUSIC))
  {
    if (m_guiState)
      m_guiState->SetPlaylistDirectory("playlistmusic://");
    // activate the playlist window if its not activated yet
    if (GetID() == CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() && iSize > 1)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
  }
}

bool CGUIWindowMusicBase::OnPlayMedia(int iItem, const std::string &player)
{
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  // party mode
  if (g_partyModeManager.IsEnabled())
  {
    PLAYLIST::CPlayList playlistTemp;
    playlistTemp.Add(pItem);
    g_partyModeManager.AddUserSongs(playlistTemp, !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICPLAYER_QUEUEBYDEFAULT));
    return true;
  }
  else if (!pItem->IsPlayList() && !pItem->IsInternetStream())
  { // single music file - if we get here then we have autoplaynextitem turned off or queuebydefault
    // turned on, but we still want to use the playlist player in order to handle more queued items
    // following etc.
    if ( (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICPLAYER_QUEUEBYDEFAULT) && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_MUSIC_PLAYLIST_EDITOR) )
    {
      //! @todo Should the playlist be cleared if nothing is already playing?
      OnQueueItem(iItem);
      return true;
    }
    pItem->SetProperty("playlist_type_hint", m_guiState->GetPlaylist());
    CServiceBroker::GetPlaylistPlayer().Play(pItem, player);
    return true;
  }
  return CGUIMediaWindow::OnPlayMedia(iItem, player);
}

/// \brief Can be overwritten to implement an own tag filling function.
/// \param items File items to fill
void CGUIWindowMusicBase::OnRetrieveMusicInfo(CFileItemList& items)
{
  // No need to attempt to read music file tags for music videos
  if (items.IsVideoDb())
    return;
  if (items.GetFolderCount()==items.Size() || items.IsMusicDb() ||
     (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICFILES_USETAGS) && !items.IsCDDA()))
  {
    return;
  }
  // Start the music info loader thread
  m_musicInfoLoader.SetProgressCallback(m_dlgProgress);
  m_musicInfoLoader.Load(items);

  bool bShowProgress = !CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog(true);
  bool bProgressVisible = false;

  auto start = std::chrono::steady_clock::now();

  while (m_musicInfoLoader.IsLoading())
  {
    if (bShowProgress)
    { // Do we have to init a progress dialog?
      auto end = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

      if (!bProgressVisible && duration.count() > 1500 && m_dlgProgress)
      { // tag loading takes more then 1.5 secs, show a progress dialog
        CURL url(items.GetPath());
        m_dlgProgress->SetHeading(CVariant{189});
        m_dlgProgress->SetLine(0, CVariant{505});
        m_dlgProgress->SetLine(1, CVariant{""});
        m_dlgProgress->SetLine(2, CVariant{url.GetWithoutUserDetails()});
        m_dlgProgress->Open();
        m_dlgProgress->ShowProgressBar(true);
        bProgressVisible = true;
      }

      if (bProgressVisible && m_dlgProgress && !m_dlgProgress->IsCanceled())
      { // keep GUI alive
        m_dlgProgress->Progress();
      }
    } // if (bShowProgress)
    KODI::TIME::Sleep(1ms);
  } // while (m_musicInfoLoader.IsLoading())

  if (bProgressVisible && m_dlgProgress)
    m_dlgProgress->Close();
}

bool CGUIWindowMusicBase::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  items.ClearArt();
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory, items);
  if (bResult)
  {
    // We want to expand disc images when browsing in file view but not on library, smartplaylist
    // or node menu music windows
    if (!items.GetPath().empty() && !StringUtils::StartsWithNoCase(items.GetPath(), "musicdb://") &&
        !StringUtils::StartsWithNoCase(items.GetPath(), "special://") &&
        !StringUtils::StartsWithNoCase(items.GetPath(), "library://"))
      CDirectory::FilterFileDirectories(items, ".iso", true);

    CMusicThumbLoader loader;
    loader.FillThumb(items);

    CQueryParams params;
    CDirectoryNode::GetDatabaseInfo(items.GetPath(), params);

    // Get art for directory when album or artist
    bool artfound = false;
    std::vector<ArtForThumbLoader> art;
    if (params.GetAlbumId() > 0)
    { // Get album and related artist(s) art
      artfound = m_musicdatabase.GetArtForItem(-1, params.GetAlbumId(), -1, false, art);
    }
    else if (params.GetArtistId() > 0)
    { // get artist art
      artfound = m_musicdatabase.GetArtForItem(-1, -1, params.GetArtistId(), true, art);
    }
    if (artfound)
    {
      std::string dirType = MediaTypeArtist;
      if (params.GetAlbumId() > 0)
        dirType = MediaTypeAlbum;
      std::map<std::string, std::string> artmap;
      for (auto artitem : art)
      {
        std::string artname;
        if (dirType == artitem.mediaType)
          artname = artitem.artType;
        else if (artitem.prefix.empty())
          artname = artitem.mediaType + "." + artitem.artType;
        else
        {
          if (dirType == MediaTypeAlbum)
            StringUtils::Replace(artitem.prefix, "albumartist", "artist");
          artname = artitem.prefix + "." + artitem.artType;
        }
      artmap.insert(std::make_pair(artname, artitem.url));
      }
      items.SetArt(artmap);
    }

    int iWindow = GetID();
    // Add "New Playlist" items when in the playlists folder, except on playlist editor screen
    if ((iWindow != WINDOW_MUSIC_PLAYLIST_EDITOR) &&
        (items.GetPath() == "special://musicplaylists/") && !items.Contains("newplaylist://"))
    {
      const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

      CFileItemPtr newPlaylist(new CFileItem(profileManager->GetUserDataItem("PartyMode.xsp"),false));
      newPlaylist->SetLabel(g_localizeStrings.Get(16035));
      newPlaylist->SetLabelPreformatted(true);
      newPlaylist->SetArt("icon", "DefaultPartyMode.png");
      newPlaylist->m_bIsFolder = true;
      items.Add(newPlaylist);

      newPlaylist.reset(new CFileItem("newplaylist://", false));
      newPlaylist->SetLabel(g_localizeStrings.Get(525));
      newPlaylist->SetArt("icon", "DefaultAddSource.png");
      newPlaylist->SetLabelPreformatted(true);
      newPlaylist->SetSpecialSort(SortSpecialOnBottom);
      newPlaylist->SetCanQueue(false);
      items.Add(newPlaylist);

      newPlaylist.reset(new CFileItem("newsmartplaylist://music", false));
      newPlaylist->SetLabel(g_localizeStrings.Get(21437));
      newPlaylist->SetArt("icon", "DefaultAddSource.png");
      newPlaylist->SetLabelPreformatted(true);
      newPlaylist->SetSpecialSort(SortSpecialOnBottom);
      newPlaylist->SetCanQueue(false);
      items.Add(newPlaylist);
    }

    // check for .CUE files here.
    items.FilterCueItems();

    std::string label;
    if (items.GetLabel().empty() && m_rootDir.IsSource(items.GetPath(), CMediaSourceSettings::GetInstance().GetSources("music"), &label))
      items.SetLabel(label);
  }

  return bResult;
}

bool CGUIWindowMusicBase::CheckFilterAdvanced(CFileItemList &items) const
{
  const std::string& content = items.GetContent();
  if ((items.IsMusicDb() || CanContainFilter(m_strFilterPath)) &&
      (StringUtils::EqualsNoCase(content, "artists") ||
       StringUtils::EqualsNoCase(content, "albums")  ||
       StringUtils::EqualsNoCase(content, "songs")))
    return true;

  return false;
}

bool CGUIWindowMusicBase::CanContainFilter(const std::string &strDirectory) const
{
  return URIUtils::IsProtocol(strDirectory, "musicdb");
}

bool CGUIWindowMusicBase::OnSelect(int iItem)
{
  auto item = m_vecItems->Get(iItem);
  if (item->IsAudioBook())
  {
    int bookmark;
    if (m_musicdatabase.GetResumeBookmarkForAudioBook(*item, bookmark) && bookmark > 0)
    {
      // find which chapter the bookmark belongs to
      auto itemIt =
          std::find_if(m_vecItems->cbegin(), m_vecItems->cend(),
                       [&](const CFileItemPtr& item) { return bookmark < item->GetEndOffset(); });

      if (itemIt != m_vecItems->cend())
      {
        // ask the user if they want to play or resume
        CContextButtons choices;
        choices.Add(MUSIC_SELECT_ACTION_PLAY, 208); // 208 = Play
        choices.Add(MUSIC_SELECT_ACTION_RESUME,
                    StringUtils::Format(g_localizeStrings.Get(12022), // 12022 = Resume from ...
                                        (*itemIt)->GetMusicInfoTag()->GetTitle()));

        auto choice = CGUIDialogContextMenu::Show(choices);
        if (choice == MUSIC_SELECT_ACTION_RESUME)
        {
          (*itemIt)->SetProperty("audiobook_bookmark", bookmark);
          return CGUIMediaWindow::OnSelect(static_cast<int>(itemIt - m_vecItems->cbegin()));
        }
        else if (choice < 0)
          return true;
      }
    }
  }

  return CGUIMediaWindow::OnSelect(iItem);
}

void CGUIWindowMusicBase::OnInitWindow()
{
  CGUIMediaWindow::OnInitWindow();
  // Prompt for rescan of library to read music file tags that were not processed by previous versions
  // and accommodate any changes to the way some tags are processed
  if (m_musicdatabase.GetMusicNeedsTagScan() != 0)
  {
    if (CServiceBroker::GetGUI()
            ->GetInfoManager()
            .GetInfoProviders()
            .GetLibraryInfoProvider()
            .GetLibraryBool(LIBRARY_HAS_MUSIC) &&
        !CMusicLibraryQueue::GetInstance().IsScanningLibrary())
    {
      // rescan of music library required
      if (CGUIDialogYesNo::ShowAndGetInput(CVariant{799}, CVariant{38060}))
      {
        int flags = CMusicInfoScanner::SCAN_RESCAN;
        // When set to fetch information on update enquire about scraping that as well
        // It may take some time, so the user may want to do it later by "Query Info For All"
        if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO))
          if (CGUIDialogYesNo::ShowAndGetInput(CVariant{799}, CVariant{38061}))
            flags |= CMusicInfoScanner::SCAN_ONLINE;

        CMusicLibraryQueue::GetInstance().ScanLibrary("", flags, true);

        m_musicdatabase.SetMusicTagScanVersion(); // once is enough (user may interrupt, but that's up to them)
      }
    }
    else
    {
      // no need to force a rescan if there's no music in the library or if a library scan is already active
      m_musicdatabase.SetMusicTagScanVersion();
    }
  }
}

std::string CGUIWindowMusicBase::GetStartFolder(const std::string &dir)
{
  std::string lower(dir); StringUtils::ToLower(lower);
  if (lower == "plugins" || lower == "addons")
    return "addons://sources/audio/";
  else if (lower == "$playlists" || lower == "playlists")
    return "special://musicplaylists/";
  return CGUIMediaWindow::GetStartFolder(dir);
}

void CGUIWindowMusicBase::OnScan(int iItem, bool bPromptRescan /*= false*/)
{
  std::string strPath;
  if (iItem < 0 || iItem >= m_vecItems->Size())
    strPath = m_vecItems->GetPath();
  else if (m_vecItems->Get(iItem)->m_bIsFolder)
    strPath = m_vecItems->Get(iItem)->GetPath();
  else
  { //! @todo MUSICDB - should we allow scanning a single item into the database?
    //!       This will require changes to the info scanner, which assumes we're running on a folder
    strPath = m_vecItems->GetPath();
  }
  // Ask for full rescan of music files when scan item from file view context menu
  bool doRescan = false;
  if (bPromptRescan)
    doRescan = CGUIDialogYesNo::ShowAndGetInput(CVariant{ 799 }, CVariant{ 38062 });

  DoScan(strPath, doRescan);
}

void CGUIWindowMusicBase::DoScan(const std::string &strPath, bool bRescan /*= false*/)
{
  if (CMusicLibraryQueue::GetInstance().IsScanningLibrary())
  {
    CMusicLibraryQueue::GetInstance().StopLibraryScanning();
    return;
  }

  // Start background loader
  int iControl=GetFocusedControlID();
  int flags = 0;
  if (bRescan)
    flags = CMusicInfoScanner::SCAN_RESCAN;
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO))
    flags |= CMusicInfoScanner::SCAN_ONLINE;

  CMusicLibraryQueue::GetInstance().ScanLibrary(strPath, flags, true);

  SET_CONTROL_FOCUS(iControl, 0);
  UpdateButtons();
}

void CGUIWindowMusicBase::OnRemoveSource(int iItem)
{

  //Remove music source from library, even when leaving songs
  CMusicDatabase database;
  database.Open();
  database.RemoveSource(m_vecItems->Get(iItem)->GetLabel());

  bool bCanceled;
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant{522}, CVariant{20340}, bCanceled, CVariant{""}, CVariant{""}, CGUIDialogYesNo::NO_TIMEOUT))
  {
    MAPSONGS songs;
    database.RemoveSongsFromPath(m_vecItems->Get(iItem)->GetPath(), songs, false);
    database.CleanupOrphanedItems();
    database.CheckArtistLinksChanged();
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetLibraryInfoProvider().ResetLibraryBools();
    m_vecItems->RemoveDiscCache(GetID());
  }
  database.Close();
}

void CGUIWindowMusicBase::OnPrepareFileItems(CFileItemList &items)
{
  CGUIMediaWindow::OnPrepareFileItems(items);

  if (!items.IsMusicDb() && !items.IsSmartPlayList())
    RetrieveMusicInfo();
}

void CGUIWindowMusicBase::OnAssignContent(const std::string& oldName, const CMediaSource& source)
{
  // Music scrapers are not source specific, so unlike video there is no content selection logic here.
  // Called on having added or edited a music source, this starts scanning items into library when required

  //! @todo: do async as updating sources for all albums could be slow??
  //Store music source in the music library, even those not scanned
  CMusicDatabase database;
  database.Open();
  database.UpdateSource(oldName, source.strName, source.strPath, source.vecPaths);
  database.Close();

  // "Add to library" yes/no dialog with additional "settings" custom button
  // "Do you want to add the media from this source to your library?"
  DialogResponse rep = DialogResponse::CHOICE_CUSTOM;
  while (rep == DialogResponse::CHOICE_CUSTOM)
  {
    rep = HELPERS::ShowYesNoCustomDialog(CVariant{20444}, CVariant{20447}, CVariant{106}, CVariant{107}, CVariant{10004});
    if (rep == DialogResponse::CHOICE_CUSTOM)
      // Edit default info provider settings so can be applied during scan
      CGUIDialogInfoProviderSettings::Show();
  }
  if (rep == DialogResponse::CHOICE_YES)
    CMusicLibraryQueue::GetInstance().ScanLibrary(source.strPath,
                                                  MUSIC_INFO::CMusicInfoScanner::SCAN_NORMAL, true);
}

