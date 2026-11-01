/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CFileItemList;

namespace KODI::PICTURE
{
/*!
 * \brief Hide the clip of each Live Photo (a .mov named like a picture next to it, which Apple's
 * maker note marks as a Live Photo) and store its path in the picture's "motionclip" property.
 *
 * It reads each picture named like a .mov. Called on the GUI thread, it waits behind the busy
 * dialog, and a pair it's cancelled before checking stays a picture and a video.
 */
void GroupLivePhotos(CFileItemList& items);

/*!
 * \brief Copy the clip embedded in a Motion Photo to a temporary file.
 * \return Path of the clip, or empty if the picture has none
 */
std::string ExtractMotionPhotoClip(const std::string& path);

/*!
 * \brief Delete the clips copied by ExtractMotionPhotoClip().
 */
void ClearMotionPhotoClips();
} // namespace KODI::PICTURE
