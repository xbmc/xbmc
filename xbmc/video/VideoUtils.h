/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/Bookmark.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>

class CFileItem;

namespace KODI::VIDEO::UTILS
{

/*! \brief
 *  Find a local trailer file for a given file item
 *  \return non-empty string with path of trailer if found
 */
std::string FindTrailer(const CFileItem& item);

/*!
 \brief Check whether an item is an optical media folder or its parent.
  This will return the non-empty path to the playable entry point of the media
  one or two levels down (VIDEO_TS.IFO for DVDs or index.bdmv for BDs).
  The returned path will be empty if folder does not meet this criterion.
 \return non-empty string if item is optical media folder, empty otherwise.
 */
std::string GetOpticalMediaPath(const CFileItem& item);

/*! \brief Check whether auto play next item is set for the media type of the given item.
  \param item [in] the item to check
  \return True if auto play next item is active, false otherwise.
  */
bool IsAutoPlayNextItem(const CFileItem& item);

/*! \brief Check whether auto play next item is set for the given content type.
  \param item [in] the content to check
  \return True if auto play next item is active, false otherwise.
  */
bool IsAutoPlayNextItem(const std::string& content);

/*! \brief Parses a playerState string from a bookmark and returns the next stack part number if available.
  \param bookmark The bookmark to parse
  \return std::nullopt if no nextpart tag, or the next part number if available.
  */
std::optional<int> GetNextPartFromBookmark(const CBookmark& bookmark);

/*!
 \brief Get the resume offset and part number for the given stack item.
 \param item The stack item to retrieve the offset for.
 \return std::nullopt if nothing found, or the part number and offset.
 */
std::optional<std::tuple<int64_t, unsigned int>> GetStackResumeOffsetAndPartNumber(
    const CFileItem& item);

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

/*!
 \brief For a given non-library folder containing video files, load info from the video database.
 \param folder The folder to load
 \return The item containing the folder including loaded info.
 */
std::shared_ptr<CFileItem> LoadVideoFilesFolderInfo(const CFileItem& folder);

} // namespace KODI::VIDEO::UTILS
