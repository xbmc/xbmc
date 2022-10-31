/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CFileItem;
class CFileItemList;

namespace VIDEO_UTILS
{
/*! \brief Start playback of the given item. If the item is a folder, build a playlist with
  all items contained in the folder and start playback of the playlist. If item is a single video
  item, start playback directly, without adding it to the video playlist first.
  \param item [in] the item to play
  */
void PlayItem(const std::shared_ptr<CFileItem>& item);

enum class QueuePosition
{
  POSITION_BEGIN, // place at begin of queue, before other items
  POSITION_END, // place at end of queue, after other items
};

/*! \brief Queue the given item in the currently active playlist. If no playlist is active,
 put the item into the video playlist.
  \param item [in] the item to queue
  \param pos [in] whether to place the item and the begin or the end of the queue
  */
void QueueItem(const std::shared_ptr<CFileItem>& item, QueuePosition pos);

/*! \brief For a given item, get the items to put in a playlist. If the item is a folder, all
+  subitems will be added recursively to the returned item list. If the item is a playlist, the
+  playlist will be loaded and contained items will be added to the returned item list. Shows a
+  busy dialog if action takes certain amount of time to give the user visual feedback.
+  \param item [in] the item to add to the playlist
+  \param queuedItems [out] the items that can be put in a play list
+  \return true on success, false otherwise
+  */
bool GetItemsForPlayList(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems);

/*!
 \brief Check whether the given item can be played by the app playlist player as one or more videos.
 \param item The item to check
 \return True if playable, false otherwise.
 */
bool IsItemPlayable(const CFileItem& item);

struct ResumeInformation
{
  bool isResumable{false}; // the playback of the item can be resumed
  int64_t startOffset{0}; // a start offset
  int partNumber{0}; // a part number
};

/*!
 \brief Check whether playback of the given item can be resumed, get detailed information.
 \param item The item to retrieve information for
 \return The resume information.
 */
ResumeInformation GetItemResumeInformation(const CFileItem& item);

} // namespace VIDEO_UTILS
