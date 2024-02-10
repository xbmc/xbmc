/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CFileItem;

namespace KODI::MUSIC
{

//! \brief Check whether an item is a cue sheet.
bool IsCUESheet(const CFileItem& item);

//! \brief Check whether an item is a lyrics file.
bool IsLyrics(const CFileItem& item);

} // namespace KODI::MUSIC
}
