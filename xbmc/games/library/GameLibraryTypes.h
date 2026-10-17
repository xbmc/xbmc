/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>
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
  DEBUG,
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
