/*
 *  Copyright (C) 2016-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Application.h"
#include "GUIPassword.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "QueueAndPlayUtils.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogBusy.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "view/GUIViewState.h"

using namespace XFILE;
using namespace std::chrono_literals;

// set and reset in worker thread, read by calling thread
CEvent CQueueAndPlayUtils::m_utilsJobEvent(false, false);

class CAddItemToPlaylistJob : public CJob
{
  const CFileItemPtr m_pItem;
  CFileItemList& m_queuedItems;

public:
  CAddItemToPlaylistJob(const CFileItemPtr& pItem, CFileItemList& queuedItems)
    : m_pItem(pItem), m_queuedItems(queuedItems)
  {
  }

  ~CAddItemToPlaylistJob(void) override = default;

  bool DoWork(void) override
  {
    CQueueAndPlayUtils queueAndPlayUtils;
    queueAndPlayUtils.JobStart();
    queueAndPlayUtils.AddToPlayList(m_pItem, m_queuedItems);
    queueAndPlayUtils.JobFinish(); // Signal job finished
    return true;
  }
};

bool CQueueAndPlayUtils::PlayItem(const CFileItemPtr& itemToPlay, bool bShowBusyDialog)
{
  if (itemToPlay->m_bIsFolder) // build a playlist and play it
  {
    XFILE::CMusicDatabaseDirectory dir;
    // Exclude any top level nodes
    XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE Node =
        dir.GetDirectoryParentType(itemToPlay->GetPath());
    if (Node == XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_OVERVIEW)
      return false;
    CFileItemList queuedItems;
    AddItemToPlayList(itemToPlay, queuedItems, bShowBusyDialog);
    CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST_MUSIC);
    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST_MUSIC, queuedItems);
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST_MUSIC);
    CServiceBroker::GetPlaylistPlayer().Play();
  }
  else // song, so just play it
  {
    PlaySong(itemToPlay);
  }
  return true;
}

bool CQueueAndPlayUtils::AddItemToPlayList(const CFileItemPtr& pItem,
                                           CFileItemList& queuedItems,
                                           bool bShowBusyDialog)
{
  int jobid = CServiceBroker::GetJobManager()->AddJob(new CAddItemToPlaylistJob(pItem, queuedItems),
                                                      nullptr, CJob::PRIORITY_NORMAL);
  if (bShowBusyDialog)
  {
    if (!CGUIDialogBusy::WaitOnEvent(m_utilsJobEvent, 500)) // 500ms before busy dialog appears
    {
      // Cancel job if its still waiting in queue (unlikely) or still running
      CServiceBroker::GetJobManager()->CancelJob(jobid);
      return false;
    }
  }
  else
  {
    while (!m_utilsJobEvent.Wait(1ms)); // wait for job to finish
  }
  return true;
}

void CQueueAndPlayUtils::PlaySong(const CFileItemPtr& itemToPlay)
{
  if (itemToPlay->HasMusicInfoTag())
    CServiceBroker::GetPlaylistPlayer().Play(itemToPlay, "");
}

void CQueueAndPlayUtils::AddToPlayList(const CFileItemPtr& pItem, CFileItemList& queuedItems)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsParentFolder())
    return;

  // fast lookup is needed here
  queuedItems.SetFastLookup(true);

  if (pItem->IsMusicDb() && pItem->m_bIsFolder && !pItem->IsParentFolder())
  { // we have a music database folder, just grab the "all" item underneath it
    XFILE::CMusicDatabaseDirectory dir;

    if (!dir.ContainsSongs(pItem->GetPath()))
    { // grab the ALL item in this category
      // Genres will still require 2 lookups, and queuing the entire Genre folder
      // will require 3 lookups (genre, artist, album)
      CMusicDbUrl musicUrl;
      if (musicUrl.FromString(pItem->GetPath()))
      {
        musicUrl.AppendPath("-1/");
        CFileItemPtr item(new CFileItem(musicUrl.ToString(), true));
        item->SetCanQueue(true); // workaround for CanQueue() check above
        AddToPlayList(item, queuedItems);
      }
      return;
    }
  }
  if (pItem->m_bIsFolder)
  {
    // Check if we add a locked share
    if (pItem->m_bIsShareOrDrive)
    {
      CFileItem item = *pItem;
      if (!g_passwordManager.IsItemUnlocked(&item, "music"))
        return;
    }

    // recursive
    CFileItemList items;
    GetDirectory(pItem->GetPath(), items);
    FormatAndSort(items);
    for (int i = 0; i < items.Size(); ++i)
      AddToPlayList(items[i], queuedItems);
  }
  else
  {
    if (pItem->IsPlayList())
    {
      std::unique_ptr<PLAYLIST::CPlayList> pPlayList(PLAYLIST::CPlayListFactory::Create(*pItem));
      if (pPlayList)
      {
        // load it
        if (!pPlayList->Load(pItem->GetPath()))
        {
          CLog::Log(LOGERROR, "{} failed to load playlist {}", __FUNCTION__, pItem->GetPath());
          return; // failed to load playlist for some reason - Log the error and fail
        }

        PLAYLIST::CPlayList playlist = *pPlayList;
        for (int i = 0; i < playlist.size(); ++i)
        {
          AddToPlayList(playlist[i], queuedItems);
        }
        return;
      }
    }
    else if (pItem->IsInternetStream() && !pItem->IsMusicDb())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(pItem);
    }
    else if (pItem->IsPlugin() && pItem->GetProperty("isplayable").asBoolean())
    {
      // python files can be played
      queuedItems.Add(pItem);
    }
    else if (!pItem->IsNFO() && (pItem->IsAudio() || pItem->IsVideo()))
    {
      const CFileItemPtr itemCheck = queuedItems.Get(pItem->GetPath());
      if (!itemCheck || itemCheck->m_lStartOffset != pItem->m_lStartOffset)
      { // add item
        const CFileItemPtr item = std::make_shared<CFileItem>(*pItem);
        m_musicDatabase.SetPropertiesForFileItem(*item);
        queuedItems.Add(item);
      }
    }
  }
}

bool CQueueAndPlayUtils::GetDirectory(const std::string& strDirectory, CFileItemList& items)
{
  CURL pathToUrl(strDirectory);
  CFileItemList cachedItems(strDirectory);
  const int winID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (!strDirectory.empty() && cachedItems.Load(winID))
  {
    items.Assign(cachedItems);
  }
  else
  {
    if (strDirectory.empty())
      SetupShares();

    CFileItemList dirItems;
    if (!GetDirectoryItems(pathToUrl, dirItems, true))
      return false;
    items.Assign(dirItems);
  }
  return true;
}

bool CQueueAndPlayUtils::GetDirectoryItems(CURL& url, CFileItemList& items, bool useDir)
{
  return m_rootDir.GetDirectory(url, items, useDir, false);
}

void CQueueAndPlayUtils::FormatAndSort(CFileItemList& items)
{
  const int winID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  const std::unique_ptr<CGUIViewState> viewState(CGUIViewState::GetViewState(winID, items));

  if (viewState)
  {
    LABEL_MASKS labelMasks;
    viewState->GetSortMethodLabelMasks(labelMasks);
    FormatItemLabels(items, labelMasks);
    items.Sort(viewState->GetSortMethod().sortBy, viewState->GetSortOrder(),
               viewState->GetSortMethod().sortAttributes);
  }
}

void CQueueAndPlayUtils::FormatItemLabels(CFileItemList& items, const LABEL_MASKS& labelMasks)
{
  const CLabelFormatter fileFormatter(labelMasks.m_strLabelFile, labelMasks.m_strLabel2File);
  const CLabelFormatter folderFormatter(labelMasks.m_strLabelFolder, labelMasks.m_strLabel2Folder);
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];

    if (pItem->IsLabelPreformatted())
      continue;

    if (pItem->m_bIsFolder)
      folderFormatter.FormatLabels(pItem.get());
    else
      fileFormatter.FormatLabels(pItem.get());
  }

  if (items.GetSortMethod() == SortByLabel)
    items.ClearSortState();
}

void CQueueAndPlayUtils::SetupShares()
{
  const CFileItemList items;
  const int winID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  CGUIViewState* viewState = CGUIViewState::GetViewState(winID, items);
  if (viewState)
  {
    m_rootDir.SetMask(viewState->GetExtensions());
    m_rootDir.SetSources(viewState->GetSources());
    delete viewState;
  }
}

void CQueueAndPlayUtils::QueueItem(const CFileItemPtr& itemIn,
                                   bool first,
                                   bool bShowBusyDialog)
{
  int playlist = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  if (playlist == PLAYLIST_NONE)
    playlist = g_application.GetAppPlayer().GetPreferredPlaylist();
  if (playlist == PLAYLIST_NONE)
    playlist = PLAYLIST_MUSIC;

  const int iOldSize = CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlist).size();

  // Check for the partymode playlist item, do nothing when "PartyMode.xsp" not exist
  if (itemIn->IsSmartPlayList())
  {
    const std::shared_ptr<CProfileManager> profileManager =
        CServiceBroker::GetSettingsComponent()->GetProfileManager();
    if ((itemIn->GetPath() == profileManager->GetUserDataItem("PartyMode.xsp")) &&
        !XFILE::CFile::Exists(itemIn->GetPath()))
      return;
  }

  if (!itemIn->CanQueue())
    itemIn->SetCanQueue(true);

  CLog::Log(LOGDEBUG, "Adding file {}{} to music playlist", itemIn->GetPath(),
            itemIn->m_bIsFolder ? " (folder) " : "");
  CFileItemList queuedItems;
  AddItemToPlayList(itemIn, queuedItems, bShowBusyDialog);

  // if party mode, add items but DONT start playing
  if (g_partyModeManager.IsEnabled())
  {
    g_partyModeManager.AddUserSongs(queuedItems, false);
    return;
  }

  if (first && g_application.GetAppPlayer().IsPlaying())
    CServiceBroker::GetPlaylistPlayer().Insert(
        playlist, queuedItems, CServiceBroker::GetPlaylistPlayer().GetCurrentSong() + 1);
  else
    CServiceBroker::GetPlaylistPlayer().Add(playlist, queuedItems);
  if (CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlist).size() &&
      !g_application.GetAppPlayer().IsPlaying())
  {
    const int winID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
    CGUIViewState* viewState = CGUIViewState::GetViewState(winID, queuedItems);
    if (viewState)
      viewState->SetPlaylistDirectory("playlistmusic://");

    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(playlist);
    CServiceBroker::GetPlaylistPlayer().Play(iOldSize, ""); // start playing at the first new item
  }
}

void CQueueAndPlayUtils::JobFinish()
{
  m_utilsJobEvent.Set(); // signal job complete
  m_musicDatabase.Close();
}

void CQueueAndPlayUtils::JobStart()
{
  m_utilsJobEvent.Reset();
  m_musicDatabase.Open();
}
