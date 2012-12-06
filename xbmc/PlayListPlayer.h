#pragma once
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

#include "guilib/IMsgTargetCallback.h"
#include <boost/shared_ptr.hpp>

#define PLAYLIST_NONE    -1
#define PLAYLIST_MUSIC   0
#define PLAYLIST_VIDEO   1
#define PLAYLIST_PICTURE 2

class CFileItem; typedef boost::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;

class CVariant;

namespace PLAYLIST
{
/*!
 \ingroup windows
 \brief Manages playlist playing.
 */
enum REPEAT_STATE { REPEAT_NONE = 0, REPEAT_ONE, REPEAT_ALL };

class CPlayList;

class CPlayListPlayer : public IMsgTargetCallback
{

public:
  CPlayListPlayer(void);
  virtual ~CPlayListPlayer(void);
  virtual bool OnMessage(CGUIMessage &message);

  /*! \brief Play the next (or another) entry in the current playlist
   \param offset The offset from the current entry (defaults to 1, i.e. the next entry).
   \param autoPlay Whether we should start playing if not already (defaults to false).
   */
  bool PlayNext(int offset = 1, bool autoPlay = false);

  /*! \brief Play the previous entry in the current playlist
   \sa PlayNext
   */
  bool PlayPrevious();
  bool PlaySongId(int songId);
  bool Play();

  /*! \brief Start playing a particular entry in the current playlist
   \param index the index of the item to play. This value is modified to ensure it lies within the current playlist.
   \param replace whether this item should replace the currently playing item. See CApplication::PlayFile (defaults to false).
   \param playPreviousOnFail whether to go back to the previous item if playback fails (default to false)
   */
  bool Play(int index, bool replace = false, bool playPreviousOnFail = false);

  /*! \brief Returns the index of the current item in active playlist.
   \return Current item in the active playlist.
   \sa SetCurrentSong
   */
  int GetCurrentSong() const;

  /*! \brief Change the current item in the active playlist.
   \param index item index in playlist. Set only if the index is within the range of the current playlist.
   \sa GetCurrentSong
   */
  void SetCurrentSong(int index);

  int GetNextSong();
  
  /*! \brief Get the index in the playlist that is offset away from the current index in the current playlist.
   Obeys any repeat settings (eg repeat one will return the current index regardless of offset)
   \return the index of the entry, or -1 if there is no current playlist. There is no guarantee that the returned index is valid.
   */
  int GetNextSong(int offset) const;

  /*! \brief Set the active playlist
   \param playList Values can be PLAYLIST_NONE, PLAYLIST_MUSIC or PLAYLIST_VIDEO
   \sa GetCurrentPlaylist
   */
  void SetCurrentPlaylist(int playlist);

  /*! \brief Get the currently active playlist
   \return PLAYLIST_NONE, PLAYLIST_MUSIC or PLAYLIST_VIDEO
   \sa SetCurrentPlaylist, GetPlaylist
   */
  int GetCurrentPlaylist() const;

  /*! \brief Get a particular playlist object
   \param playList Values can be PLAYLIST_MUSIC or PLAYLIST_VIDEO
   \return A reference to the CPlayList object.
   \sa GetCurrentPlaylist
   */
  CPlayList& GetPlaylist(int playlist);
  const CPlayList& GetPlaylist(int iPlaylist) const;

  /*! \brief Removes any item from all playlists located on a removable share
   \return Number of items removed from PLAYLIST_MUSIC and PLAYLIST_VIDEO
   */
  int RemoveDVDItems();

  /*! \brief Resets the current song and unplayable counts.
   Does not alter the active playlist.
  */
  void Reset();

  void ClearPlaylist(int iPlaylist);
  void Clear();

  /*! \brief Set shuffle state of a playlist.
   If the shuffle state changes, the playlist is shuffled or unshuffled.
   Has no effect if Party Mode is enabled.
   \param playlist the playlist to (un)shuffle, PLAYLIST_MUSIC or PLAYLIST_VIDEO.
   \param shuffle set true to shuffle, false to unshuffle.
   \param notify notify the user with a Toast notification (defaults to false)
   \sa IsShuffled
   */
  void SetShuffle(int playlist, bool shuffle, bool notify = false);
  
  /*! \brief Return whether a playlist is shuffled.
   If partymode is enabled, this always returns false.
   \param playlist the playlist to query for shuffle state, PLAYLIST_MUSIC or PLAYLIST_VIDEO.
   \return true if the given playlist is shuffled and party mode isn't enabled, false otherwise.
   \sa SetShuffle
   */
  bool IsShuffled(int iPlaylist) const;

  /*! \brief Return whether or not something has been played yet from the current playlist.
   \return true if something has been played, false otherwise.
   */
  bool HasPlayedFirstFile() const;

  /*! \brief Set repeat state of a playlist.
   If called while in Party Mode, repeat is disabled.
   \param playlist the playlist to set repeat state for, PLAYLIST_MUSIC or PLAYLIST_VIDEO.
   \param state set to REPEAT_NONE, REPEAT_ONE or REPEAT_ALL
   \param notify notify the user with a Toast notification
   \sa GetRepeat
   */
  void SetRepeat(int iPlaylist, REPEAT_STATE state, bool notify = false);
  REPEAT_STATE GetRepeat(int iPlaylist) const;

  // add items via the playlist player
  void Add(int iPlaylist, CPlayList& playlist);
  void Add(int iPlaylist, const CFileItemPtr &pItem);
  void Add(int iPlaylist, CFileItemList& items);
  void Insert(int iPlaylist, CPlayList& playlist, int iIndex);
  void Insert(int iPlaylist, const CFileItemPtr &pItem, int iIndex);
  void Insert(int iPlaylist, CFileItemList& items, int iIndex);
  void Remove(int iPlaylist, int iPosition);
  void Swap(int iPlaylist, int indexItem1, int indexItem2);
protected:
  /*! \brief Returns true if the given is set to repeat all
   \param playlist Playlist to be query
   \return true if the given playlist is set to repeat all, false otherwise.
   */
  bool Repeated(int playlist) const;

  /*! \brief Returns true if the given is set to repeat one
   \param playlist Playlist to be query
   \return true if the given playlist is set to repeat one, false otherwise.
   */
  bool RepeatedOne(int playlist) const;

  void ReShuffle(int iPlaylist, int iPosition);

  void AnnouncePropertyChanged(int iPlaylist, const std::string &strProperty, const CVariant &value);

  bool m_bPlayedFirstFile;
  bool m_bPlaybackStarted;
  int m_iFailedSongs;
  unsigned int m_failedSongsStart;
  int m_iCurrentSong;
  int m_iCurrentPlayList;
  CPlayList* m_PlaylistMusic;
  CPlayList* m_PlaylistVideo;
  CPlayList* m_PlaylistEmpty;
  REPEAT_STATE m_repeatState[2];
};

}

/*!
 \ingroup windows
 \brief Global instance of playlist player
 */
extern PLAYLIST::CPlayListPlayer g_playlistPlayer;
