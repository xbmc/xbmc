/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>

class CFileItem;

namespace KODI::ART
{

enum class AdditionalIdentifiers : uint8_t
{
  NONE,
  SEASON_AND_EPISODE, // Append SxxEyy to the file name
  PLAYLIST, // Append playlist number to the file name
};

//! \brief Set default icon for item.
void FillInDefaultIcon(CFileItem& item);

/*!
 * \brief Get the folder image associated with item.
 * \param item Item to get folder image for
 * \param folderJPG Thumb file to use
 * \return Folder thumb file appropriate for item
 */
std::string GetFolderThumb(const CFileItem& item, const std::string& folderJPG = "folder.jpg");

/*! \brief Assemble the filename of a particular piece of local artwork for an item.
           No file existence check is typically performed.
 \param artFile the art file to search for.
 \param useFolder whether to look in the folder for the art file. Defaults to false.
 \param additionalIdentifiers for multi-episode/multi-movie files. Append SxxEyy or playlist number to the file name.
                              Defaults to none.
 \return the path to the local artwork.
 \sa FindLocalArt
 */
std::string GetLocalArt(const CFileItem& item,
                        const std::string& artFile,
                        bool useFolder = false,
                        AdditionalIdentifiers useSeasonAndEpisode = AdditionalIdentifiers::NONE);

/*!
 \brief Assemble the base filename of local artwork for an item,
 accounting for archives, stacks and multi-paths, and BDMV/VIDEO_TS folders.
 \param useFolder whether to look in the folder for the art file. Defaults to false.
 \param additionalIdentifiers for multi-episode/multi-movie files. Append SxxEyy or playlist number to the file name.
                              Defaults to none.
 \return the path to the base filename for artwork lookup.
 \sa GetLocalArt
 */
std::string GetLocalArtBaseFilename(
    const CFileItem& item,
    bool& useFolder,
    AdditionalIdentifiers additionalIdentifiers = AdditionalIdentifiers::NONE);

/*!
 \brief Get the local fanart for item if it exists
 \return path to the local fanart for this item, or empty if none exists
 \sa GetFolderThumb, GetTBNFile
 */
std::string GetLocalFanart(const CFileItem& item);

/*! \brief Get the .tbn file associated with an item.
 \param item CFileItem containing the item path.
 \param season For multi-episode files. Append SxxEyy to the file name.
 \param episode For multi-episode files. Append SxxEyy to the file name.
 \return the path to the .tbn file
*/
std::string GetTBNFile(const CFileItem& item, int season = -1, int episode = -1);

} // namespace KODI::ART
