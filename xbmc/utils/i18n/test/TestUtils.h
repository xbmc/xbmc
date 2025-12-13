/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ostream>

namespace KODI::UTILS::I18N
{
struct Bcp47Extension;
struct ParsedBcp47Tag;

std::ostream& operator<<(std::ostream& os, const Bcp47Extension& obj);
std::ostream& operator<<(std::ostream& os, const ParsedBcp47Tag& obj);
} // namespace KODI::UTILS::I18N
