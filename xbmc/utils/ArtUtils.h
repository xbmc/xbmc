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

// Gets the .tbn file associated with an item
std::string GetTBNFile(const CFileItem& item);

} // namespace KODI::ART
