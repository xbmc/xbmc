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

//! \brief Check whether an item is a video item.
//! \details Note that this returns true for anything with a video info tag,
//!          so that may include eg. folders.
bool IsVideo(const CFileItem& item);

} // namespace KODI::VIDEO
