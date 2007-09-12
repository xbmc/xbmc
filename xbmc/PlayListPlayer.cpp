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
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "application.h"
#include "util.h"
#include "PartyModeManager.h"

using namespace PLAYLIST;

CPlayListPlayer g_playlistPlayer;

CPlayListPlayer::CPlayListPlayer(void)
{
  m_iCurrentSong = -1;
  m_bChanged = false;
  m_bPlayedFirstFile = false;
  m_iCurrentPlayList = PLAYLIST_NONE;
  for (int i = 0; i < 2; i++)
    m_repeatState[i] = REPEAT_NONE;
  m_iFailedSongs = 0;
}

CPlayListPlayer::~CPlayListPlayer(void)
{
  m_PlaylistMusic.Clear();
  m_PlaylistVideo.Clear();
  m_PlaylistEmpty.Clear();
}

bool CPlayListPlayer::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetLPVOID())
    {
      // update our item if necessary
      CPlayList &playlist = GetPlaylist(m_iCurrentPlayList);
      playlist.UpdateItem((CFileItem *)message.GetLPVOID());
    }
    break;
  case GUI_MSG_PLAYBACK_STOPPED:
    {
      if (m_iCurrentPlayList != PLAYLIST_NONE)
      {
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
        m_gWindowManager.SendThreadMessage(msg);
        Reset();
        m_iCurrentPlayList = PLAYLIST_NONE;
        return true;
      }
    }
    break;
  }

  return false;
}

int CPlayListPlayer::GetNextSong()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return -1;
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return -1;
  int iSong = m_iCurrentSong;

  // party mode
  if (g_partyModeManager.IsEnabled() && GetCurrentPlaylist() == PLAYLIST_MUSIC)
    return iSong + 1;

  // if repeat one, keep playing the current song if its valid
  if (RepeatedOne(m_iCurrentPlayList))
  {
    // otherwise immediately abort playback
    if (playlist[m_iCurrentSong].IsUnPlayable())
    {
      CLog::Log(LOGERROR,"Playlist Player: RepeatOne stuck on unplayable item: %i, path [%s]", m_iCurrentSong, playlist[m_iCurrentSong].m_strPath.c_str());
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      m_gWindowManager.SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = PLAYLIST_NONE;
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

/// \brief Play next entry in current playlist
void CPlayListPlayer::PlayNext(bool bAutoPlay)
{
  int iSong = GetNextSong();
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);

  // stop playing
  if ((iSong < 0) || (iSong >= playlist.size()) || (playlist.GetPlayable() <= 0))
  {
    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
    m_gWindowManager.SendThreadMessage(msg);
    Reset();
    m_iCurrentPlayList = PLAYLIST_NONE;
    return;
  }

  if (bAutoPlay)
  {
    const CPlayList::CPlayListItem& item = playlist[iSong];
    if ( item.IsShoutCast() )
    {
      return ;
    }
  }
  Play(iSong, bAutoPlay);
  if (playlist[iSong].IsAudio() && m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
    m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);
  //g_partyModeManager.OnSongChange();
}

/// \brief Play previous entry in current playlist
void CPlayListPlayer::PlayPrevious()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return ;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) return ;
  int iSong = m_iCurrentSong;

  if (!RepeatedOne(m_iCurrentPlayList))
    iSong--;

  if (iSong < 0)
    iSong = playlist.size() - 1;

  Play(iSong, false, true);
}

void CPlayListPlayer::Play()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) return;

  Play(0);
}

/// \brief Start playing entry \e iSong in current playlist
/// \param iSong Song in playlist
void CPlayListPlayer::Play(int iSong, bool bAutoPlay /* = false */, bool bPlayPrevious /* = false */)
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return ;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) return ;
  if (iSong < 0) iSong = 0;
  if (iSong >= playlist.size()) iSong = playlist.size() - 1;


  // check if the item itself is a playlist, and can be expanded
  // only allow a few levels, this could end up in a loop
  // if they refer to each other in a loop
  for(int i=0;i<5;i++)
  {
    if(!playlist.Expand(iSong))
      break;
  }

  m_bChanged = true;
  int iPreviousSong = m_iCurrentSong;
  m_iCurrentSong = iSong;
  const CPlayList::CPlayListItem& item = playlist[m_iCurrentSong];
  playlist.SetPlayed(true);

  if (!g_application.PlayFile(item, bAutoPlay))
  {
    CLog::Log(LOGERROR,"Playlist Player: skipping unplayable item: %i, path [%s]", m_iCurrentSong, item.m_strPath.c_str());
    playlist.SetUnPlayable(m_iCurrentSong);

    // abort on 100 failed CONSECTUTIVE songs
    m_iFailedSongs++;
    if (m_iFailedSongs >= 100)
    {
      CLog::Log(LOGDEBUG,"Playlist Player: too many consecutive failures... aborting playback");

      // open error dialog
      CGUIDialogOK::ShowAndGetInput(257, 16026, 16027, 0);

      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      m_gWindowManager.SendThreadMessage(msg);
      Reset();
      GetPlaylist(m_iCurrentPlayList).Clear();
      m_iCurrentPlayList = PLAYLIST_NONE;
      m_iFailedSongs = 0;
      return;
    }

    // how many playable items are in the playlist?
    if (playlist.GetPlayable() > 0)
    {
      if (bPlayPrevious)
        PlayPrevious();
      else
        PlayNext();
      return;
    }
    // none? then abort playback
    else
    {
      CLog::Log(LOGDEBUG,"Playlist Player: no more playable items... aborting playback");
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      m_gWindowManager.SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = PLAYLIST_NONE;
      return;
    }
  }

  m_bPlayedFirstFile = true;

  // consecutive error counter so reset if the current item is playing
  m_iFailedSongs = 0;

  if (!item.IsShoutCast())
  {
    if (iPreviousSong < 0)
    {
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STARTED, 0, 0, m_iCurrentPlayList, m_iCurrentSong, (LPVOID)&item);
      m_gWindowManager.SendThreadMessage( msg );
    }
    else
    {
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, m_iCurrentPlayList, MAKELONG(m_iCurrentSong, iPreviousSong), (LPVOID)&item);
      m_gWindowManager.SendThreadMessage(msg);
    }
  }
}

/// \brief Change the current song in playlistplayer.
/// \param iSong Song in playlist
void CPlayListPlayer::SetCurrentSong(int iSong)
{
  if (iSong >= -1 && iSong < GetPlaylist(m_iCurrentPlayList).size())
    m_iCurrentSong = iSong;
}

/// \brief Returns to current song in active playlist.
/// \return Current song
int CPlayListPlayer::GetCurrentSong() const
{
  return m_iCurrentSong;
}

bool CPlayListPlayer::HasChanged()
{
  bool bResult = m_bChanged;
  m_bChanged = false;
  return bResult;
}

/// \brief Returns the active playlist.
/// \return Active playlist \n
/// Return values can be: \n
/// - PLAYLIST_NONE \n No playlist active
/// - PLAYLIST_MUSIC \n Playlist from music playlist window
/// - PLAYLIST_VIDEO \n Playlist from music playlist window
int CPlayListPlayer::GetCurrentPlaylist()
{
  return m_iCurrentPlayList;
}

/// \brief Set active playlist.
/// \param iPlayList Playlist to set active \n
/// Values can be: \n
/// - PLAYLIST_NONE \n No playlist active
/// - PLAYLIST_MUSIC \n Playlist from music playlist window
/// - PLAYLIST_VIDEO \n Playlist from music playlist window
void CPlayListPlayer::SetCurrentPlaylist(int iPlaylist)
{
  if (iPlaylist == m_iCurrentPlayList)
    return;

  // changing the current playlist while party mode is on
  // disables party mode
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  m_iCurrentPlayList = iPlaylist;
  m_bPlayedFirstFile = false;
  m_bChanged = true;
}

void CPlayListPlayer::ClearPlaylist(int iPlaylist)
{
  // clear our applications playlist file
  g_application.m_strPlayListFile.Empty();

  CPlayList& playlist = GetPlaylist(iPlaylist);
  playlist.Clear();

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendMessage(msg);
}

/// \brief Get the playlist object specified in \e nPlayList
/// \param nPlayList Values can be: \n
/// - PLAYLIST_MUSIC \n Playlist from music playlist window
/// - PLAYLIST_VIDEO \n Playlist from music playlist window
/// \return A reference to the CPlayList object.
CPlayList& CPlayListPlayer::GetPlaylist(int iPlaylist)
{
  switch ( iPlaylist )
  {
  case PLAYLIST_MUSIC:
    return m_PlaylistMusic;
    break;
  case PLAYLIST_VIDEO:
    return m_PlaylistVideo;
    break;
  default:
    m_PlaylistEmpty.Clear();
    return m_PlaylistEmpty;
    break;
  }
}

/// \brief Removes any item from all playlists located on a removable share
/// \return Number of items removed from PLAYLIST_MUSIC and PLAYLIST_VIDEO
int CPlayListPlayer::RemoveDVDItems()
{
  int nRemovedM = m_PlaylistMusic.RemoveDVDItems();
  int nRemovedV = m_PlaylistVideo.RemoveDVDItems();

  return nRemovedM + nRemovedV;
}

/// \brief Resets the playlistplayer, but the active playlist stays the same.
void CPlayListPlayer::Reset()
{
  m_iCurrentSong = -1;
  m_bPlayedFirstFile = false;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendMessage(msg);
}

/// \brief Whether or not something has been played yet or not from the current playlist.
bool CPlayListPlayer::HasPlayedFirstFile()
{
  return m_bPlayedFirstFile;
}

/// \brief Returns \e true if iPlaylist is repeated
/// \param iPlaylist Playlist to be asked
bool CPlayListPlayer::Repeated(int iPlaylist)
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO)
    return (m_repeatState[iPlaylist] == REPEAT_ALL);
  return false;
}

/// \brief Returns \e true if iPlaylist repeats one song
/// \param iPlaylist Playlist to be asked
bool CPlayListPlayer::RepeatedOne(int iPlaylist)
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO)
    return (m_repeatState[iPlaylist] == REPEAT_ONE);
  return false;
}

/// \brief Shuffle play the current playlist
/// \param bYesNo To Enable shuffle play, set to \e true
void CPlayListPlayer::SetShuffle(int iPlaylist, bool bYesNo)
{
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return;

  // disable shuffle in party mode
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    return;

  // do we even need to do anything?
  if (bYesNo != IsShuffled(iPlaylist))
  {
    // save the order value of the current song so we can use it find its new location later
    int iOrder = -1;
    if (m_iCurrentSong >= 0)
      iOrder = GetPlaylist(iPlaylist)[m_iCurrentSong].m_iprogramCount;

    // shuffle or unshuffle as necessary
    if (bYesNo)
      GetPlaylist(iPlaylist).Shuffle();
    else
      GetPlaylist(iPlaylist).UnShuffle();

    // find the previous order value and fix the current song marker
    if (iOrder >= 0)
    {
      int iIndex = GetPlaylist(iPlaylist).FindOrder(iOrder);
      if (iIndex >= 0)
        m_iCurrentSong = iIndex;
      // if iIndex < 0, something unexpected happened
      // so dont do anything
    }
  }
}

bool CPlayListPlayer::IsShuffled(int iPlaylist)
{
  // even if shuffled, party mode says its not
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    return false;

  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO)
  {
    return GetPlaylist(iPlaylist).IsShuffled();
  }
  return false;
}

void CPlayListPlayer::SetRepeat(int iPlaylist, REPEAT_STATE state)
{
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return;

  // disable repeat in party mode
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    state = REPEAT_NONE;

  m_repeatState[iPlaylist] = state;
}

REPEAT_STATE CPlayListPlayer::GetRepeat(int iPlaylist)
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO)
    return m_repeatState[iPlaylist];
  return REPEAT_NONE;
}

void CPlayListPlayer::ReShuffle(int iPlaylist, int iPosition)
{
  // playlist has not played yet so shuffle the entire list
  // (this only really works for new video playlists)
  if (!GetPlaylist(iPlaylist).WasPlayed())
  {
    GetPlaylist(iPlaylist).Shuffle();
  }
  // we're trying to shuffle new items into the curently playing playlist
  // so we shuffle starting at two positions below the current item
  else if (iPlaylist == m_iCurrentPlayList)
  {
    if (
      (g_application.IsPlayingAudio() && iPlaylist == PLAYLIST_MUSIC) ||
      (g_application.IsPlayingVideo() && iPlaylist == PLAYLIST_VIDEO)
      )
    {
	    g_playlistPlayer.GetPlaylist(iPlaylist).Shuffle(m_iCurrentSong + 2);
    }
  }
  // otherwise, shuffle from the passed position
  // which is the position of the first new item added
  else
  {
    g_playlistPlayer.GetPlaylist(iPlaylist).Shuffle(iPosition);
  }
}

void CPlayListPlayer::Add(int iPlaylist, CPlayList::CPlayListItem& item)
{
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(item);
	if (list.IsShuffled())
		ReShuffle(iPlaylist, iSize);
}

void CPlayListPlayer::Add(int iPlaylist, CPlayList& playlist)
{
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(playlist);
	if (list.IsShuffled())
		ReShuffle(iPlaylist, iSize);
}

void CPlayListPlayer::Add(int iPlaylist, CFileItem *pItem)
{
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(pItem);
	if (list.IsShuffled())
		ReShuffle(iPlaylist, iSize);
}

void CPlayListPlayer::Add(int iPlaylist, CFileItemList& items)
{
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(items);
	if (list.IsShuffled())
		ReShuffle(iPlaylist, iSize);
}
