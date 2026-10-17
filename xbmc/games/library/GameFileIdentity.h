/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameLibraryTypes.h"

#include <cstdint>
#include <string>

class CFileItemList;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Work out what identifies a game file to a catalogue
 *
 * Cartridge-sized files are hashed whole. Archives are hashed on their
 * largest member, since that is the ROM and the catalogues hash the ROM,
 * not the archive. Disc images are never hashed whole: the serial their
 * boot sector or system file carries is read instead, the way RetroArch and
 * the catalogues themselves identify discs.
 *
 * Every read is bounded. A file that cannot be read is left unidentified,
 * never a reason to stop a scan.
 */
class CGameFileIdentity
{
public:
  /*!
   * \brief Files larger than this are treated as disc images and read only
   *        as far as their header
   */
  static constexpr uint64_t MAX_HASH_SIZE = 64 * 1024 * 1024;

  /*!
   * \brief How far into a disc image its header is looked for before giving up
   */
  static constexpr uint64_t MAX_DISC_SCAN = 2ULL * 1024 * 1024 * 1024;

  /*!
   * \brief Fill in what can be learnt about a file
   *
   * \param path The game file, which may be an archive or a cue sheet
   * \param[out] file Size and, where obtainable, crc32, md5, serial
   * \param media The platform's media format, which decides whether a
   *        serial is worth looking for
   *
   * \return True if at least one identity was found
   */
  static bool Identify(const std::string& path, GameFile& file, MediaFormat media);

  /*!
   * \brief The serial a disc image's header carries, or empty
   *
   * Reads PlayStation (SYSTEM.CNF), Saturn, Mega-CD and Dreamcast headers.
   * The result is normalised to upper-case letters and digits, as the
   * catalogues key on.
   *
   * \param maxRead How far into the image to look for the header
   */
  static std::string ReadDiscSerial(const std::string& trackPath, uint64_t maxRead = MAX_DISC_SCAN);

  //! \brief The serial a disc header holds, once the header has been found
  static std::string SerialFromHeader(const std::string& data, size_t at, int console);

  /*!
   * \brief The first data track named by a cue sheet or GDI file, or empty
   */
  static std::string FirstTrack(const std::string& sheetPath);

  /*!
   * \brief A serial as the catalogues key on it
   */
  static std::string NormaliseSerial(std::string serial);

private:
  static bool HashWhole(const std::string& path, GameFile& file);
  static bool HashArchive(const std::string& path, GameFile& file);
  static bool HashLargestMember(const std::string& path, GameFile& file);
  static std::string ReadArchivedDiscSerial(const std::string& path);

  //! \brief The files in an archive, without opening any of them
  static bool ArchiveFiles(const std::string& path, CFileItemList& files);

  /*!
   * \brief Every file in a zip, from its directory, without decompressing any
   *
   * An arcade set is identified by the CRCs of all its chips, which the zip
   * already records.
   */
  static bool ListZip(const std::string& path, std::vector<ArchiveMember>& members);
};
} // namespace GAME
} // namespace KODI
