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
 * \brief A container for art work. key: art type, value: URL of art file
 */
using ArtWork = std::map<std::string, std::string, std::less<>>;

/*!
 * \brief A container for art work for multipe seasons. key: season number, value: art work
 */
using SeasonsArtWork = std::map<int, ArtWork>;

} // namespace KODI::ART
