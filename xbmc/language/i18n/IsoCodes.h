/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>

namespace KODI::LANGUAGE::I18N
{
//! The length of an alpha-2 code: ISO 639-1, and ISO 3166-1 alpha-2
inline constexpr std::size_t ALPHA2_CODE_LENGTH{2};

//! The length of an alpha-3 code: ISO 639-2 in either form, and ISO 3166-1 alpha-3
inline constexpr std::size_t ALPHA3_CODE_LENGTH{3};
} // namespace KODI::LANGUAGE::I18N
