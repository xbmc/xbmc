/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/library/GameLibraryTypes.h"

#include <optional>
#include <string_view>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 * \brief Read a name from a numbered scene release list
 *
 * The DS and GBA scenes number every release, and sets built from their
 * release lists, such as ADVANsCEne's, are named de facto as
 *
 *   2797 Kimi no Yusha (JP)
 *   0414 My Pet Hotel (EU)(M2)
 *
 * The number stays in the title, since one name can't tell it from a title
 * that starts with a number, "2002 FIFA World Cup (US)".
 */
class CSceneNaming
{
public:
  CSceneNaming() = delete;

  /*!
   * \brief The region a code names, or nothing
   *
   * \param code The code as the lists write it: "JP", "EU", "KS"
   */
  static std::optional<GameRegion> Region(std::string_view code);
};
} // namespace GAME
} // namespace KODI
