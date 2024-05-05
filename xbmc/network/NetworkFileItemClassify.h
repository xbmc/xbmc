/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CFileItem;

namespace KODI::NETWORK
{

//! \brief Check whether an item is a an internet stream.
bool IsInternetStream(const CFileItem& item, const bool bStrictCheck = false);

//! \brief Check whether an item is on a remote location.
bool IsRemote(const CFileItem& item);

//! \brief Check whether an item is on a streamed filesystem.
bool IsStreamedFilesystem(const CFileItem& item);
} // namespace KODI::NETWORK
