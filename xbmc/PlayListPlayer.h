/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/IMsgTargetCallback.h"
#include "messaging/IMessageTarget.h"
#include "playlists/PlayListTypes.h"

#include <chrono>
#include <map>
#include <memory>

class CAction;
class CFileItem; typedef std::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;

class CVariant;

namespace PLAYLIST
{
class CPlayList;

class CPlayListPlayer : public IMsgTargetCallback,
                        public KODI::MESSAGING::IMessageTarget
{

public:
  CPlayListPlayer(void);
  ~CPlayListPlayer(void) override;
  bool OnMessage(CGUIMessage &message) override;

  int GetMessageMask() override;
  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

  /*! \brief Play the next (or another) entry in the current playlist
   \param offset The offset from the current entry (defaults to 1, i.e. the next entry).
   \param autoPlay Whether we should start playing if not already (defaults to false).
   */
  bool PlayNext(int offset = 1, bool autoPlay = false);

  /*! \brief Play the previous entry in the current playlist
   \sa PlayNext
   */
  bool PlayPrevious();
  bool PlayItemIdx(int itemIdx);
  bool Play();

  /*!
   * \brief Creates a new playlist for an item and starts playing it
   * \param pItem The item to play.
   * \param player The player name.
   * \param forceSelection for Blurays, force simple menu to change playlist (default to false)
   * \return True if has success, otherwise false.
   */
  bool Play(const CFileItemPtr& pItem, const std::string& player, bool forceSelection = false);

  /*! \brief Start playing a particular entry in the current playlist
   \param index the index of the item to play. This value is modified to ensure it lies within the current playlist.
   \param replace whether this item should replace the currently playing item. See CApplication::PlayFile (defaults to false).
   \param playPreviousOnFail whether to go back to the previous item if playback fails (default to false)
   \param forceSelection for Blurays, force simple menu to change playlist (default to false)
   */
  bool Play(int index,
            const std::string& player,
            bool replace = false,
            bool playPreviousOnFail = false,
            bool forceSelection = false);

  /*! \brief Returns the index of the current item in active playlist.
   \return Current item in the active playlist.
   \sa SetCurrentItemIdx
   */
  int GetCurrentItemIdx() const;

  /*! \brief Change the current item in the active playlist.
   \param index item index in playlist. Set only if the index is within the range of the current playlist.
   \sa GetCurrentItemIdx
   */
  void SetCurrentItemIdx(int index);

  int GetNextItemIdx();

  /*! \brief Get the index in the playlist that is offset away from the current index in the current playlist.
   Obeys any repeat settings (eg repeat one will return the current index regardless of offset)
   \return the index of the entry, or -1 if there is no current playlist. There is no guarantee that the returned index is valid.
   */
  int GetNextItemIdx(int offset) const;

  /*! \brief Set the active playlist
   \param id Values can be PLAYLIST::TYPE_NONE, PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO
   \sa GetCurrentPlaylist
   */
  void SetCurrentPlaylist(PLAYLIST::Id playlistId);

  /*! \brief Get the currently active playlist
   \return PLAYLIST::TYPE_NONE, PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO
   \sa SetCurrentPlaylist, GetPlaylist
   */
  PLAYLIST::Id GetCurrentPlaylist() const;

  /*! \brief Get a particular playlist object
   \param id Values can be PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO
   \return A reference to the CPlayList object.
   \sa GetCurrentPlaylist
   */
  CPlayList& GetPlaylist(PLAYLIST::Id playlistId);
  const CPlayList& GetPlaylist(PLAYLIST::Id playlistId) const;

  /*! \brief Removes any item from all playlists located on a removable share
   \return Number of items removed from PLAYLIST::TYPE_MUSIC and PLAYLIST::TYPE_VIDEO
   */
  int RemoveDVDItems();

  /*! \brief Resets the current song and unplayable counts.
   Does not alter the active playlist.
  */
  void Reset();

  void ClearPlaylist(PLAYLIST::Id playlistId);
  void Clear();

  /*! \brief Set shuffle state of a playlist.
   If the shuffle state changes, the playlist is shuffled or unshuffled.
   Has no effect if Party Mode is enabled.
   \param playlist the playlist to (un)shuffle, PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO.
   \param shuffle set true to shuffle, false to unshuffle.
   \param notify notify the user with a Toast notification (defaults to false)
   \sa IsShuffled
   */
  void SetShuffle(PLAYLIST::Id playlistId, bool shuffle, bool notify = false);

  /*! \brief Return whether a playlist is shuffled.
   If partymode is enabled, this always returns false.
   \param playlist the playlist to query for shuffle state, PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO.
   \return true if the given playlist is shuffled and party mode isn't enabled, false otherwise.
   \sa SetShuffle
   */
  bool IsShuffled(PLAYLIST::Id playlistId) const;

  /*! \brief Return whether or not something has been played yet from the current playlist.
   \return true if something has been played, false otherwise.
   */
  bool HasPlayedFirstFile() const;

  /*! \brief Set repeat state of a playlist.
   If called while in Party Mode, repeat is disabled.
   \param playlist the playlist to set repeat state for, PLAYLIST::TYPE_MUSIC or PLAYLIST::TYPE_VIDEO.
   \param state set to RepeatState::NONE, RepeatState::ONE or RepeatState::ALL
   \param notify notify the user with a Toast notification
   \sa GetRepeat
   */
  void SetRepeat(PLAYLIST::Id playlistId, PLAYLIST::RepeatState state, bool notify = false);
  PLAYLIST::RepeatState GetRepeat(PLAYLIST::Id playlistId) const;

  // add items via the playlist player
  void Add(PLAYLIST::Id playlistId, const CPlayList& playlist);
  void Add(PLAYLIST::Id playlistId, const CFileItemPtr& pItem);
  void Add(PLAYLIST::Id playlistId, const CFileItemList& items);
  void Insert(PLAYLIST::Id playlistId, const CPlayList& playlist, int iIndex);
  void Insert(PLAYLIST::Id playlistId, const CFileItemPtr& pItem, int iIndex);
  void Insert(PLAYLIST::Id playlistId, const CFileItemList& items, int iIndex);
  void Remove(PLAYLIST::Id playlistId, int iPosition);
  void Swap(PLAYLIST::Id playlistId, int indexItem1, int indexItem2);

  bool IsSingleItemNonRepeatPlaylist() const;

  bool OnAction(const CAction &action);
protected:
  /*! \brief Returns true if the given is set to repeat all
   \param playlist Playlist to be query
   \return true if the given playlist is set to repeat all, false otherwise.
   */
  bool Repeated(PLAYLIST::Id playlistId) const;

  /*! \brief Returns true if the given is set to repeat one
   \param playlist Playlist to be query
   \return true if the given playlist is set to repeat one, false otherwise.
   */
  bool RepeatedOne(PLAYLIST::Id playlistId) const;

  void ReShuffle(PLAYLIST::Id playlistId, int iPosition);

  void AnnouncePropertyChanged(PLAYLIST::Id playlistId,
                               const std::string& strProperty,
                               const CVariant& value);

  bool m_bPlayedFirstFile;
  bool m_bPlaybackStarted;
  int m_iFailedSongs;
  std::chrono::time_point<std::chrono::steady_clock> m_failedSongsStart;
  int m_iCurrentSong;
  PLAYLIST::Id m_iCurrentPlayList{PLAYLIST::TYPE_NONE};
  CPlayList* m_PlaylistMusic;
  CPlayList* m_PlaylistVideo;
  CPlayList* m_PlaylistEmpty;
  std::map<PLAYLIST::Id, PLAYLIST::RepeatState> m_repeatState{
      {PLAYLIST::TYPE_MUSIC, PLAYLIST::RepeatState::NONE},
      {PLAYLIST::TYPE_VIDEO, PLAYLIST::RepeatState::NONE},
      {PLAYLIST::TYPE_PICTURE, PLAYLIST::RepeatState::NONE},
  };
};

}
