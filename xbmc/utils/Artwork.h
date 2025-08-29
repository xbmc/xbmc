/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>

namespace KODI::ART
{
/*!
 * \brief A container for artwork. key: artwork type, value: URL of artwork file
 */
using Artwork = std::map<std::string, std::string, std::less<>>;

/*!
 * \brief A container for artwork for multiple seasons. key: season number, value: artwork
 */
using SeasonsArtwork = std::map<int, Artwork>;

} // namespace KODI::ART
