/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CFileItem;

namespace KODI::VIDEO
{

//! \brief Check whether an item is a blu-ray file.
bool IsBDFile(const CFileItem& item);

//! \brief Check whether an item is a disc stub.
bool IsDiscStub(const CFileItem& item);

//! \brief Check whether an item is a DVD file.
bool IsDVDFile(const CFileItem& item, bool bVobs = true, bool bIfos = true);

//! \brief Checks whether item points to a protected blu-ray disc.
bool IsProtectedBlurayDisc(const CFileItem& item);

//! \brief Checks whether item points to a blu-ray playlist (.mpls)
bool IsBlurayPlaylist(const CFileItem& item);

//! \brief Check whether an item is a subtitle file.
bool IsSubtitle(const CFileItem& item);

//! \brief Check whether an item is a video item.
//! \details Note that this returns true for anything with a video info tag,
//!          so that may include eg. folders.
bool IsVideo(const CFileItem& item);

//! \brief Is the item a video asset, excluding folders
//! \param[in] item the item
//! \return true if it is, false otherwise
bool IsVideoAssetFile(const CFileItem& item);

//! \brief Check whether an item is a video database item.
bool IsVideoDb(const CFileItem& item);

//! \brief Check whether an item is a video extras folder item.
bool IsVideoExtrasFolder(const CFileItem& item);

} // namespace KODI::VIDEO
