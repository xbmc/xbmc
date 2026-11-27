/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaType.h"
#include "utils/ContentUtils.h"

#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;

namespace MUSIC_UTILS
{
/*! \brief Show a dialog to allow the selection of type of art from a list.
  Input is a fileitem list, with each item having an "arttype" property
  e.g. "thumb", current art URL (if art exists), and label. One of these art types
  can be selected, or a new art type added. The new art type is added as a new item
  in the list, as well as returned as the selected art type.
  \param artitems [in/out] a fileitem list to display
  \return the selected art type e.g. "fanart" or empty string when cancelled.
  \sa FillArtTypesList
  */
std::string ShowSelectArtTypeDialog(CFileItemList& artitems);

/*! \brief Helper function to build a list of art types for a music library item.
  This fetches the possible types of art for a song, album or artist, and the
  current art URL (if the item has art of that type), for display on a dialog.
  \param musicitem a music CFileItem (song, album or artist)
  \param artlist [out] a fileitem list,  each item having "arttype" property
  e.g. "thumb", current art URL (if art exists), and localized label (for common arttypes)
  \return true if art types are retrieved, false if none is found.
  \sa ShowSelectArtTypeDialog
  */
bool FillArtTypesList(CFileItem& musicitem, CFileItemList& artlist);

/*! \brief Helper function to asynchronously update art in the music database
  and then refresh the album & artist art of the currently playing song.
  For the song, album or artist this adds a job to the queue to update the art table
  modifying, adding or deleting that type of art. Changes to album or artist art are
  then passed to the currently playing song (if there is one).
  \param pItem a shared pointer to a music CFileItem (song, album or artist)
  \param strType the type of art e.g. "fanart" or "thumb" etc.
  \param strArt art URL, when empty the entry for that type of art is deleted.
  */
void UpdateArtJob(const std::shared_ptr<CFileItem>& pItem,
                  const std::string& strType,
                  const std::string& strArt);

/*! \brief Show a dialog to allow the selection of user rating.
  \param iSelected the rating to show initially
  \return the selected rating, 0 (no rating), 1 to 10 or -1 no rating selected
  */
int ShowSelectRatingDialog(int iSelected);

/*! \brief Helper function to asynchronously update the user rating of a song
  \param pItem pointer to song item being rated
  \param userrating the userrating 0 = no rating, 1 to 10
  */
void UpdateSongRatingJob(const std::shared_ptr<CFileItem>& pItem, int userrating);

/*! \brief Get the types of art for an artist or album that are to be
  automatically fetched from local files during scanning
  \param mediaType [in] artist or album
  \return vector of art types that are to be fetched during scanning
  */
std::vector<std::string> GetArtTypesToScan(KODI::MEDIA::TYPE mediaType);

/*! \brief Validate string is acceptable as the name of an additional art type
  - limited length, and ascii alphanumeric characters only
  \param potentialArtType [in] potential art type name
  \return true if the art type is valid
  */
bool IsValidArtType(const std::string& potentialArtType);

/*! \brief Check whether auto play next item is set for the given item.
  \param item [in] the item to check
  \return True if auto play next item is active, false otherwise.
  */
bool IsAutoPlayNextItem(const CFileItem& item);

/*! \brief Start playback of the given item. If the item is a folder, build a playlist with
  all items contained in the folder and start playback of the playlist. If item is a single music
  item, it replaces the music playlist and plays.
  \param item [in] the item to play
  \param player [in] the player to use, empty for default player
  \param mode [in] queue all successors and play them after item
  */
void PlayItem(const std::shared_ptr<CFileItem>& item,
              const std::string& player,
              ContentUtils::PlayMode mode = ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM);

enum class QueuePosition
{
  POSITION_BEGIN, // play next if the playlist is playing, otherwise at the end
  POSITION_END, // at the end of the playlist
};

/*! \brief Queue the given item on the music playlist, or on the video playlist while audio
  follows what it plays. Start playback of the playlist, if player is not already playing.
  \param item [in] the item to queue
  \param pos [in] whether to play the item next or add it at the end
  */
void QueueItem(const std::shared_ptr<CFileItem>& item, QueuePosition pos);

/*! \brief For a given item, get the items to put in a playlist. If the item is a folder, all
  subitems will be added recursively to the returned item list. If the item is a playlist, the
  playlist will be loaded and contained items will be added to the returned item list. Shows a
  busy dialog if action takes certain amount of time to give the user visual feedback.
  \param item [in] the item to add to the playlist
  \param queuedItems [out] the items that can be put in a play list
  \param startAt [in] the item to start at, if any
  \param startPosition [out] where the start landed in queuedItems, or -1 if nowhere
  \return true on success, false otherwise
  */
bool GetItemsForPlayList(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems,
                         const std::shared_ptr<CFileItem>& startAt = nullptr,
                         int* startPosition = nullptr);

/*!
 \brief Check whether the given item can be played on the music playlist as one or more songs.
 \param item The item to check
 \return True if playable, false otherwise.
 */
bool IsItemPlayable(const CFileItem& item);

} // namespace MUSIC_UTILS
