/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListPlayer.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/VideoDatabaseFile.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

using namespace PLAYLIST;
using namespace KODI::MESSAGING;

CPlayListPlayer::CPlayListPlayer(void)
{
  m_PlaylistMusic = new CPlayList(TYPE_MUSIC);
  m_PlaylistVideo = new CPlayList(TYPE_VIDEO);
  m_PlaylistEmpty = new CPlayList;
  m_iCurrentSong = -1;
  m_bPlayedFirstFile = false;
  m_bPlaybackStarted = false;
  m_iFailedSongs = 0;
  m_failedSongsStart = std::chrono::steady_clock::now();
}

CPlayListPlayer::~CPlayListPlayer(void)
{
  Clear();
  delete m_PlaylistMusic;
  delete m_PlaylistVideo;
  delete m_PlaylistEmpty;
}

bool CPlayListPlayer::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PREV_ITEM && !IsSingleItemNonRepeatPlaylist())
  {
    PlayPrevious();
    return true;
  }
  else if (action.GetID() == ACTION_NEXT_ITEM && !IsSingleItemNonRepeatPlaylist())
  {
    PlayNext();
    return true;
  }
  else
    return false;
}

bool CPlayListPlayer::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
    {
      // update the items in our playlist(s) if necessary
      for (Id playlistId : {TYPE_MUSIC, TYPE_VIDEO})
      {
        CPlayList& playlist = GetPlaylist(playlistId);
        CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
        playlist.UpdateItem(item.get());
      }
    }
    break;
  case GUI_MSG_PLAYBACK_STOPPED:
    {
      if (m_iCurrentPlayList != TYPE_NONE && m_bPlaybackStarted)
      {
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        Reset();
        m_iCurrentPlayList = TYPE_NONE;
        return true;
      }
    }
    break;
  case GUI_MSG_PLAYBACK_STARTED:
    {
      m_bPlaybackStarted = true;
    }
    break;
  }

  return false;
}

int CPlayListPlayer::GetNextSong(int offset) const
{
  if (m_iCurrentPlayList == TYPE_NONE)
    return -1;

  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return -1;

  int song = m_iCurrentSong;

  // party mode
  if (g_partyModeManager.IsEnabled() && GetCurrentPlaylist() == TYPE_MUSIC)
    return song + offset;

  // wrap around in the case of repeating
  if (RepeatedOne(m_iCurrentPlayList))
    return song;

  song += offset;
  if (song >= playlist.size() && Repeated(m_iCurrentPlayList))
    song %= playlist.size();

  return song;
}

int CPlayListPlayer::GetNextSong()
{
  if (m_iCurrentPlayList == TYPE_NONE)
    return -1;
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return -1;
  int iSong = m_iCurrentSong;

  // party mode
  if (g_partyModeManager.IsEnabled() && GetCurrentPlaylist() == TYPE_MUSIC)
    return iSong + 1;

  // if repeat one, keep playing the current song if its valid
  if (RepeatedOne(m_iCurrentPlayList))
  {
    // otherwise immediately abort playback
    if (m_iCurrentSong >= 0 && m_iCurrentSong < playlist.size() && playlist[m_iCurrentSong]->GetProperty("unplayable").asBoolean())
    {
      CLog::Log(LOGERROR, "Playlist Player: RepeatOne stuck on unplayable item: {}, path [{}]",
                m_iCurrentSong, playlist[m_iCurrentSong]->GetPath());
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = TYPE_NONE;
      return -1;
    }
    return iSong;
  }

  // if we've gone beyond the playlist and repeat all is enabled,
  // then we clear played status and wrap around
  iSong++;
  if (iSong >= playlist.size() && Repeated(m_iCurrentPlayList))
    iSong = 0;

  return iSong;
}

bool CPlayListPlayer::PlayNext(int offset, bool bAutoPlay)
{
  int iSong = GetNextSong(offset);
  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);

  if ((iSong < 0) || (iSong >= playlist.size()) || (playlist.GetPlayable() <= 0))
  {
    if(!bAutoPlay)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(34201));

    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
    Reset();
    m_iCurrentPlayList = TYPE_NONE;
    return false;
  }

  return Play(iSong, "", false);
}

bool CPlayListPlayer::PlayPrevious()
{
  if (m_iCurrentPlayList == TYPE_NONE)
    return false;

  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  int iSong = m_iCurrentSong;

  if (!RepeatedOne(m_iCurrentPlayList))
    iSong--;

  if (iSong < 0 && Repeated(m_iCurrentPlayList))
    iSong = playlist.size() - 1;

  if (iSong < 0 || playlist.size() <= 0)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(34202));
    return false;
  }

  return Play(iSong, "", false, true);
}

bool CPlayListPlayer::IsSingleItemNonRepeatPlaylist() const
{
  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  return (playlist.size() <= 1 && !RepeatedOne(m_iCurrentPlayList) && !Repeated(m_iCurrentPlayList));
}

bool CPlayListPlayer::Play()
{
  if (m_iCurrentPlayList == TYPE_NONE)
    return false;

  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return false;

  return Play(0, "");
}

bool CPlayListPlayer::PlaySongId(int songId)
{
  if (m_iCurrentPlayList == TYPE_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return Play();

  for (int i = 0; i < playlist.size(); i++)
  {
    if (playlist[i]->HasMusicInfoTag() && playlist[i]->GetMusicInfoTag()->GetDatabaseId() == songId)
      return Play(i, "");
  }
  return Play();
}

bool CPlayListPlayer::Play(const CFileItemPtr& pItem, const std::string& player)
{
  Id playlistId;
  bool isVideo{pItem->IsVideo()};
  bool isAudio{pItem->IsAudio()};

  if (isAudio && !isVideo)
    playlistId = TYPE_MUSIC;
  else if (isVideo && !isAudio)
    playlistId = TYPE_VIDEO;
  else if (pItem->HasProperty("playlist_type_hint"))
  {
    // There are two main cases that can fall here:
    // - If an extension is set on both audio / video extension lists example .strm
    //   see GetFileExtensionProvider() -> GetVideoExtensions() / GetAudioExtensions()
    //   When you play the .strm containing single path, cause that
    //   IsVideo() and IsAudio() methods both return true
    //
    // - When you play a playlist (e.g. .m3u / .strm) containing multiple paths,
    //   and the path played is generic (e.g.without extension) and have no properties
    //   to detect the media type, IsVideo() / IsAudio() both return false
    //
    // for these cases the type is unknown so we rely on the hint
    playlistId = pItem->GetProperty("playlist_type_hint").asInteger32(TYPE_NONE);
  }
  else
  {
    CLog::LogF(LOGWARNING, "ListItem type must be audio or video type. The type can be specified "
                           "by using ListItem::getVideoInfoTag or ListItem::getMusicInfoTag, in "
                           "the case of playlist entries by adding #KODIPROP mimetype value.");
    return false;
  }

  ClearPlaylist(playlistId);
  Reset();
  SetCurrentPlaylist(playlistId);
  Add(playlistId, pItem);

  return Play(0, player);
}

bool CPlayListPlayer::Play(int iSong,
                           const std::string& player,
                           bool bAutoPlay /* = false */,
                           bool bPlayPrevious /* = false */)
{
  if (m_iCurrentPlayList == TYPE_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return false;
  if (iSong < 0)
    iSong = 0;
  if (iSong >= playlist.size())
    iSong = playlist.size() - 1;

  // check if the item itself is a playlist, and can be expanded
  // only allow a few levels, this could end up in a loop
  // if they refer to each other in a loop
  for (int i=0; i<5; i++)
  {
    if(!playlist.Expand(iSong))
      break;
  }

  m_iCurrentSong = iSong;
  CFileItemPtr item = playlist[m_iCurrentSong];
  if (item->IsVideoDb() && !item->HasVideoInfoTag())
    *(item->GetVideoInfoTag()) = XFILE::CVideoDatabaseFile::GetVideoTag(CURL(item->GetDynPath()));

  playlist.SetPlayed(true);

  m_bPlaybackStarted = false;

  const auto playAttempt = std::chrono::steady_clock::now();
  bool ret = g_application.PlayFile(*item, player, bAutoPlay);
  if (!ret)
  {
    CLog::Log(LOGERROR, "Playlist Player: skipping unplayable item: {}, path [{}]", m_iCurrentSong,
              CURL::GetRedacted(item->GetDynPath()));
    playlist.SetUnPlayable(m_iCurrentSong);

    // abort on 100 failed CONSECUTIVE songs
    if (!m_iFailedSongs)
      m_failedSongsStart = playAttempt;
    m_iFailedSongs++;
    const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_failedSongsStart);

    if ((m_iFailedSongs >= advancedSettings->m_playlistRetries &&
         advancedSettings->m_playlistRetries >= 0) ||
        ((duration.count() >=
          static_cast<unsigned int>(advancedSettings->m_playlistTimeout) * 1000) &&
         advancedSettings->m_playlistTimeout))
    {
      CLog::Log(LOGDEBUG,"Playlist Player: one or more items failed to play... aborting playback");

      // open error dialog
      HELPERS::ShowOKDialogText(CVariant{16026}, CVariant{16027});

      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      Reset();
      GetPlaylist(m_iCurrentPlayList).Clear();
      m_iCurrentPlayList = TYPE_NONE;
      m_iFailedSongs = 0;
      m_failedSongsStart = std::chrono::steady_clock::now();
      return false;
    }

    // how many playable items are in the playlist?
    if (playlist.GetPlayable() > 0)
    {
      return bPlayPrevious ? PlayPrevious() : PlayNext();
    }
    // none? then abort playback
    else
    {
      CLog::Log(LOGDEBUG,"Playlist Player: no more playable items... aborting playback");
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = TYPE_NONE;
      return false;
    }
  }

  // reset the start offset of this item
  if (item->GetStartOffset() == STARTOFFSET_RESUME)
    item->SetStartOffset(0);

  //! @todo - move the above failure logic and the below success logic
  //!        to callbacks instead so we don't rely on the return value
  //!        of PlayFile()

  // consecutive error counter so reset if the current item is playing
  m_iFailedSongs = 0;
  m_failedSongsStart = std::chrono::steady_clock::now();
  m_bPlayedFirstFile = true;
  return true;
}

void CPlayListPlayer::SetCurrentSong(int iSong)
{
  if (iSong >= -1 && iSong < GetPlaylist(m_iCurrentPlayList).size())
    m_iCurrentSong = iSong;
}

int CPlayListPlayer::GetCurrentSong() const
{
  return m_iCurrentSong;
}

Id CPlayListPlayer::GetCurrentPlaylist() const
{
  return m_iCurrentPlayList;
}

void CPlayListPlayer::SetCurrentPlaylist(Id playlistId)
{
  if (playlistId == m_iCurrentPlayList)
    return;

  // changing the current playlist while party mode is on
  // disables party mode
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  m_iCurrentPlayList = playlistId;
  m_bPlayedFirstFile = false;
}

void CPlayListPlayer::ClearPlaylist(Id playlistId)
{
  // clear our applications playlist file
  g_application.m_strPlayListFile.clear();

  CPlayList& playlist = GetPlaylist(playlistId);
  playlist.Clear();

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

CPlayList& CPlayListPlayer::GetPlaylist(Id playlistId)
{
  switch (playlistId)
  {
    case TYPE_MUSIC:
      return *m_PlaylistMusic;
      break;
    case TYPE_VIDEO:
      return *m_PlaylistVideo;
      break;
    default:
      m_PlaylistEmpty->Clear();
      return *m_PlaylistEmpty;
      break;
  }
}

const CPlayList& CPlayListPlayer::GetPlaylist(Id playlistId) const
{
  switch (playlistId)
  {
    case TYPE_MUSIC:
      return *m_PlaylistMusic;
      break;
    case TYPE_VIDEO:
      return *m_PlaylistVideo;
      break;
    default:
      // NOTE: This playlist may not be empty if the caller of the non-const version alters it!
      return *m_PlaylistEmpty;
      break;
  }
}

int CPlayListPlayer::RemoveDVDItems()
{
  int nRemovedM = m_PlaylistMusic->RemoveDVDItems();
  int nRemovedV = m_PlaylistVideo->RemoveDVDItems();

  return nRemovedM + nRemovedV;
}

void CPlayListPlayer::Reset()
{
  m_iCurrentSong = -1;
  m_bPlayedFirstFile = false;
  m_bPlaybackStarted = false;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

bool CPlayListPlayer::HasPlayedFirstFile() const
{
  return m_bPlayedFirstFile;
}

bool CPlayListPlayer::Repeated(Id playlistId) const
{
  const auto repStatePos = m_repeatState.find(playlistId);
  if (repStatePos != m_repeatState.end())
    return repStatePos->second == RepeatState::ALL;
  return false;
}

bool CPlayListPlayer::RepeatedOne(Id playlistId) const
{
  const auto repStatePos = m_repeatState.find(playlistId);
  if (repStatePos != m_repeatState.end())
    return (repStatePos->second == RepeatState::ONE);
  return false;
}

void CPlayListPlayer::SetShuffle(Id playlistId, bool bYesNo, bool bNotify /* = false */)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;

  // disable shuffle in party mode
  if (g_partyModeManager.IsEnabled() && playlistId == TYPE_MUSIC)
    return;

  // do we even need to do anything?
  if (bYesNo != IsShuffled(playlistId))
  {
    // save the order value of the current song so we can use it find its new location later
    int iOrder = -1;
    CPlayList& playlist = GetPlaylist(playlistId);
    if (m_iCurrentSong >= 0 && m_iCurrentSong < playlist.size())
      iOrder = playlist[m_iCurrentSong]->m_iprogramCount;

    // shuffle or unshuffle as necessary
    if (bYesNo)
      playlist.Shuffle();
    else
      playlist.UnShuffle();

    if (bNotify)
    {
      std::string shuffleStr =
          StringUtils::Format("{}: {}", g_localizeStrings.Get(191),
                              g_localizeStrings.Get(bYesNo ? 593 : 591)); // Shuffle: All/Off
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559),  shuffleStr);
    }

    // find the previous order value and fix the current song marker
    if (iOrder >= 0)
    {
      int iIndex = playlist.FindOrder(iOrder);
      if (iIndex >= 0)
        m_iCurrentSong = iIndex;
      // if iIndex < 0, something unexpected happened
      // so dont do anything
    }
  }

  // its likely that the playlist changed
  if (CServiceBroker::GetGUI() != nullptr)
  {
    CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }

  AnnouncePropertyChanged(playlistId, "shuffled", IsShuffled(playlistId));
}

bool CPlayListPlayer::IsShuffled(Id playlistId) const
{
  // even if shuffled, party mode says its not
  if (g_partyModeManager.IsEnabled() && playlistId == TYPE_MUSIC)
    return false;

  if (playlistId == TYPE_MUSIC || playlistId == TYPE_VIDEO)
    return GetPlaylist(playlistId).IsShuffled();

  return false;
}

void CPlayListPlayer::SetRepeat(Id playlistId, RepeatState state, bool bNotify /* = false */)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;

  // disable repeat in party mode
  if (g_partyModeManager.IsEnabled() && playlistId == TYPE_MUSIC)
    state = RepeatState::NONE;

  // notify the user if there was a change in the repeat state
  if (m_repeatState[playlistId] != state && bNotify)
  {
    int iLocalizedString;
    if (state == RepeatState::NONE)
      iLocalizedString = 595; // Repeat: Off
    else if (state == RepeatState::ONE)
      iLocalizedString = 596; // Repeat: One
    else
      iLocalizedString = 597; // Repeat: All
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(iLocalizedString));
  }

  m_repeatState[playlistId] = state;

  CVariant data;
  switch (state)
  {
    case RepeatState::ONE:
      data = "one";
      break;
    case RepeatState::ALL:
      data = "all";
      break;
    default:
      data = "off";
      break;
  }

  // its likely that the playlist changed
  if (CServiceBroker::GetGUI() != nullptr)
  {
    CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }

  AnnouncePropertyChanged(playlistId, "repeat", data);
}

RepeatState CPlayListPlayer::GetRepeat(Id playlistId) const
{
  const auto repStatePos = m_repeatState.find(playlistId);
  if (repStatePos != m_repeatState.end())
    return repStatePos->second;
  return RepeatState::NONE;
}

void CPlayListPlayer::ReShuffle(Id playlistId, int iPosition)
{
  // playlist has not played yet so shuffle the entire list
  // (this only really works for new video playlists)
  if (!GetPlaylist(playlistId).WasPlayed())
  {
    GetPlaylist(playlistId).Shuffle();
  }
  // we're trying to shuffle new items into the currently playing playlist
  // so we shuffle starting at two positions below the current item
  else if (playlistId == m_iCurrentPlayList)
  {
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if ((appPlayer->IsPlayingAudio() && playlistId == TYPE_MUSIC) ||
        (appPlayer->IsPlayingVideo() && playlistId == TYPE_VIDEO))
    {
      GetPlaylist(playlistId).Shuffle(m_iCurrentSong + 2);
    }
  }
  // otherwise, shuffle from the passed position
  // which is the position of the first new item added
  else
  {
    GetPlaylist(playlistId).Shuffle(iPosition);
  }
}

void CPlayListPlayer::Add(Id playlistId, const CPlayList& playlist)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;
  CPlayList& list = GetPlaylist(playlistId);
  int iSize = list.size();
  list.Add(playlist);
  if (list.IsShuffled())
    ReShuffle(playlistId, iSize);
}

void CPlayListPlayer::Add(Id playlistId, const CFileItemPtr& pItem)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;
  CPlayList& list = GetPlaylist(playlistId);
  int iSize = list.size();
  list.Add(pItem);
  if (list.IsShuffled())
    ReShuffle(playlistId, iSize);
}

void CPlayListPlayer::Add(Id playlistId, const CFileItemList& items)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;
  CPlayList& list = GetPlaylist(playlistId);
  int iSize = list.size();
  list.Add(items);
  if (list.IsShuffled())
    ReShuffle(playlistId, iSize);

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CPlayListPlayer::Insert(Id playlistId, const CPlayList& playlist, int iIndex)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;
  CPlayList& list = GetPlaylist(playlistId);
  int iSize = list.size();
  list.Insert(playlist, iIndex);
  if (list.IsShuffled())
    ReShuffle(playlistId, iSize);
  else if (m_iCurrentPlayList == playlistId && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;
}

void CPlayListPlayer::Insert(Id playlistId, const CFileItemPtr& pItem, int iIndex)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;
  CPlayList& list = GetPlaylist(playlistId);
  int iSize = list.size();
  list.Insert(pItem, iIndex);
  if (list.IsShuffled())
    ReShuffle(playlistId, iSize);
  else if (m_iCurrentPlayList == playlistId && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;
}

void CPlayListPlayer::Insert(Id playlistId, const CFileItemList& items, int iIndex)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;
  CPlayList& list = GetPlaylist(playlistId);
  int iSize = list.size();
  list.Insert(items, iIndex);
  if (list.IsShuffled())
    ReShuffle(playlistId, iSize);
  else if (m_iCurrentPlayList == playlistId && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CPlayListPlayer::Remove(Id playlistId, int iPosition)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;
  CPlayList& list = GetPlaylist(playlistId);
  list.Remove(iPosition);
  if (m_iCurrentPlayList == playlistId && m_iCurrentSong >= iPosition)
    m_iCurrentSong--;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CPlayListPlayer::Clear()
{
  if (m_PlaylistMusic)
    m_PlaylistMusic->Clear();
  if (m_PlaylistVideo)
    m_PlaylistVideo->Clear();
  if (m_PlaylistEmpty)
    m_PlaylistEmpty->Clear();
}

void CPlayListPlayer::Swap(Id playlistId, int indexItem1, int indexItem2)
{
  if (playlistId != TYPE_MUSIC && playlistId != TYPE_VIDEO)
    return;

  CPlayList& list = GetPlaylist(playlistId);
  if (list.Swap(indexItem1, indexItem2) && playlistId == m_iCurrentPlayList)
  {
    if (m_iCurrentSong == indexItem1)
      m_iCurrentSong = indexItem2;
    else if (m_iCurrentSong == indexItem2)
      m_iCurrentSong = indexItem1;
  }

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CPlayListPlayer::AnnouncePropertyChanged(Id playlistId,
                                              const std::string& strProperty,
                                              const CVariant& value)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  if (strProperty.empty() || value.isNull() ||
      (playlistId == TYPE_VIDEO && !appPlayer->IsPlayingVideo()) ||
      (playlistId == TYPE_MUSIC && !appPlayer->IsPlayingAudio()))
    return;

  CVariant data;
  data["player"]["playerid"] = playlistId;
  data["property"][strProperty] = value;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPropertyChanged",
                                                     data);
}

int PLAYLIST::CPlayListPlayer::GetMessageMask()
{
  return TMSG_MASK_PLAYLISTPLAYER;
}

void PLAYLIST::CPlayListPlayer::OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  auto wakeScreensaver = []() {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPower = components.GetComponent<CApplicationPowerHandling>();
    appPower->ResetScreenSaver();
    appPower->WakeUpScreenSaverAndDPMS();
  };

  switch (pMsg->dwMessage)
  {
  case TMSG_PLAYLISTPLAYER_PLAY:
    if (pMsg->param1 != -1)
      Play(pMsg->param1, "");
    else
      Play();
    break;

  case TMSG_PLAYLISTPLAYER_PLAY_SONG_ID:
    if (pMsg->param1 != -1)
    {
      bool *result = (bool*)pMsg->lpVoid;
      *result = PlaySongId(pMsg->param1);
    }
    else
      Play();
    break;

  case TMSG_PLAYLISTPLAYER_NEXT:
    PlayNext();
    break;

  case TMSG_PLAYLISTPLAYER_PREV:
    PlayPrevious();
    break;

  case TMSG_PLAYLISTPLAYER_ADD:
    if (pMsg->lpVoid)
    {
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);

      Add(pMsg->param1, (*list));
      delete list;
    }
    break;

  case TMSG_PLAYLISTPLAYER_INSERT:
    if (pMsg->lpVoid)
    {
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);
      Insert(pMsg->param1, (*list), pMsg->param2);
      delete list;
    }
    break;

  case TMSG_PLAYLISTPLAYER_REMOVE:
    if (pMsg->param1 != -1)
      Remove(pMsg->param1, pMsg->param2);
    break;

  case TMSG_PLAYLISTPLAYER_CLEAR:
    ClearPlaylist(pMsg->param1);
    break;

  case TMSG_PLAYLISTPLAYER_SHUFFLE:
    SetShuffle(pMsg->param1, pMsg->param2 > 0);
    break;

  case TMSG_PLAYLISTPLAYER_REPEAT:
    SetRepeat(pMsg->param1, static_cast<RepeatState>(pMsg->param2));
    break;

  case TMSG_PLAYLISTPLAYER_GET_ITEMS:
    if (pMsg->lpVoid)
    {
      PLAYLIST::CPlayList playlist = GetPlaylist(pMsg->param1);
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);

      for (int i = 0; i < playlist.size(); i++)
        list->Add(std::make_shared<CFileItem>(*playlist[i]));
    }
    break;

  case TMSG_PLAYLISTPLAYER_SWAP:
    if (pMsg->lpVoid)
    {
      auto indexes = static_cast<std::vector<int>*>(pMsg->lpVoid);
      if (indexes->size() == 2)
        Swap(pMsg->param1, indexes->at(0), indexes->at(1));
      delete indexes;
    }
    break;

  case TMSG_MEDIA_PLAY:
  {
    wakeScreensaver();

    // first check if we were called from the PlayFile() function
    if (pMsg->lpVoid && pMsg->param2 == 0)
    {
      // Discard the current playlist, if TMSG_MEDIA_PLAY gets posted with just a single item.
      // Otherwise items may fail to play, when started while a playlist is playing.
      Reset();

      CFileItem *item = static_cast<CFileItem*>(pMsg->lpVoid);
      g_application.PlayFile(*item, "", pMsg->param1 != 0);
      delete item;
      return;
    }

    //g_application.StopPlaying();
    // play file
    if (pMsg->lpVoid)
    {
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);

      if (list->Size() > 0)
      {
        Id playlistId = TYPE_MUSIC;
        for (int i = 0; i < list->Size(); i++)
        {
          if ((*list)[i]->IsVideo())
          {
            playlistId = TYPE_VIDEO;
            break;
          }
        }

        ClearPlaylist(playlistId);
        SetCurrentPlaylist(playlistId);
        if (list->Size() == 1 && !(*list)[0]->IsPlayList())
        {
          CFileItemPtr item = (*list)[0];
          // if the item is a plugin we need to resolve the URL to ensure the infotags are filled.
          if (URIUtils::HasPluginPath(*item) &&
              !XFILE::CPluginDirectory::GetResolvedPluginResult(*item))
          {
            return;
          }
          if (!item->IsPVR() && (item->IsAudio() || item->IsVideo()))
            Play(item, pMsg->strParam);
          else
            g_application.PlayMedia(*item, pMsg->strParam, playlistId);
        }
        else
        {
          // Handle "shuffled" option if present
          if (list->HasProperty("shuffled") && list->GetProperty("shuffled").isBoolean())
            SetShuffle(playlistId, list->GetProperty("shuffled").asBoolean(), false);
          // Handle "repeat" option if present
          if (list->HasProperty("repeat") && list->GetProperty("repeat").isInteger())
            SetRepeat(playlistId, static_cast<RepeatState>(list->GetProperty("repeat").asInteger()),
                      false);

          Add(playlistId, (*list));
          Play(pMsg->param1, pMsg->strParam);
        }
      }

      delete list;
    }
    else if (pMsg->param1 == TYPE_MUSIC || pMsg->param1 == TYPE_VIDEO)
    {
      if (GetCurrentPlaylist() != pMsg->param1)
        SetCurrentPlaylist(pMsg->param1);

      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_PLAY, pMsg->param2);
    }
  }
  break;

  case TMSG_MEDIA_RESTART:
    g_application.Restart(true);
    break;

  case TMSG_MEDIA_STOP:
  {
    // restore to previous window if needed
    bool stopSlideshow = true;
    bool stopVideo = true;
    bool stopMusic = true;

    Id playlistId = pMsg->param1;
    if (playlistId != TYPE_NONE)
    {
      stopSlideshow = (playlistId == TYPE_PICTURE);
      stopVideo = (playlistId == TYPE_VIDEO);
      stopMusic = (playlistId == TYPE_MUSIC);
    }

    if ((stopSlideshow && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW) ||
      (stopVideo && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO) ||
      (stopVideo && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME) ||
      (stopMusic && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION))
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

    wakeScreensaver();

    // stop playing file
    if (appPlayer->IsPlaying())
      g_application.StopPlaying();
  }
  break;

  case TMSG_MEDIA_PAUSE:
    if (appPlayer->HasPlayer())
    {
      wakeScreensaver();
      appPlayer->Pause();
    }
    break;

  case TMSG_MEDIA_UNPAUSE:
    if (appPlayer->IsPausedPlayback())
    {
      wakeScreensaver();
      appPlayer->Pause();
    }
    break;

  case TMSG_MEDIA_PAUSE_IF_PLAYING:
    if (appPlayer->IsPlaying() && !appPlayer->IsPaused())
    {
      wakeScreensaver();
      appPlayer->Pause();
    }
    break;

  case TMSG_MEDIA_SEEK_TIME:
  {
    if (appPlayer->IsPlaying() || appPlayer->IsPaused())
      appPlayer->SeekTime(pMsg->param3);

    break;
  }
  default:
    break;
  }
}
