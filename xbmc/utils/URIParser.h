/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string_view>

namespace KODI::UTILS::URIParser
{
/*!
 * \brief Check whether a string matches the RFC 3986 generic URI syntax
 * \param uri String to validate
 * \return true if the complete string matches the generic URI grammar, false otherwise
 *
 * Individual URI schemes can impose additional restrictions that are not checked here.
 */
bool IsURI(std::string_view uri);
} // namespace KODI::UTILS::URIParser
