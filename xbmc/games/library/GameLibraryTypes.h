/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/Territory.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 * \brief What a platform's games are distributed on
 */
enum class MediaFormat
{
  UNKNOWN,
  CARTRIDGE,
  DISC,
  TAPE,
  DISK,
  CARD,
  DOWNLOAD,
  OTHER,
};

/*!
 * \ingroup games
 * \brief How finished a release was when it was dumped
 */
enum class ReleaseStatus
{
  RETAIL,
  BETA,
  PROTOTYPE,
  SAMPLE,
  DEMO,
  ALPHA,
  KIOSK,
  DEBUG_BUILD,
  PROGRAM,
};

/*!
 * \ingroup games
 * \brief Under whose authority a release was published
 */
enum class Licence
{
  LICENSED,
  UNLICENSED,
  AFTERMARKET,
  HOMEBREW,
  PIRATE,
};

/*!
 * \ingroup games
 * \brief Where a release was sold, as its file name gives it
 *
 * A country or area is a territory: "US", or "150" for Europe as UN M.49
 * codes it. A market no territory names exactly keeps the convention's own
 * word: No-Intro's "World" is the three big release markets, not every
 * country, and its "Scandinavia" isn't Northern Europe.
 */
struct GameRegion
{
  LANGUAGE::CTerritory territory;
  std::string market;

  //! \brief A region from a territory's code, or a market's name when territory is empty
  static GameRegion Make(std::string_view territory, std::string_view market = {})
  {
    return {LANGUAGE::CTerritory::FromCode(territory), std::string(market)};
  }

  //! \brief The territory's code, or the market's name
  std::string ToString() const
  {
    return territory.ToString().empty() ? market : territory.ToString();
  }

  bool IsEmpty() const { return territory.ToString().empty() && market.empty(); }

  bool operator==(const GameRegion& other) const = default;
};

/*!
 * \ingroup games
 * \brief One file inside a zip, as the zip's own directory describes it
 */
struct ArchiveMember
{
  std::string name;
  uint64_t size{0};
  std::string crc32;
};

/*!
 * \ingroup games
 * \brief One file on disk belonging to a release
 */
struct GameFile
{
  std::string path;
  uint64_t size{0};
  std::string crc32;
  std::string md5;
  std::string sha256; // not computed yet
  std::string serial;
  std::vector<ArchiveMember> members; // a zip's files, when it holds more than one
};
} // namespace GAME
} // namespace KODI
