/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CFileItem;

namespace KODI::ART
{

/*!
 \brief Get the local fanart for item if it exists
 \return path to the local fanart for this item, or empty if none exists
 \sa GetFolderThumb, GetTBNFile
 */
std::string GetLocalFanart(const CFileItem& item);

// Gets the .tbn file associated with an item
std::string GetTBNFile(const CFileItem& item);

} // namespace KODI::ART
