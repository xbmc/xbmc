/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
*/

#pragma once

#include <optional>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class CURL;
namespace XFILE
{
class CFile;
}

struct ChplChapter
{
  uint64_t start_pts_ms = 0; // milliseconds
  std::string title;
};

enum class ChplChapterStatus : int
{
  Error = -1,
  Found = 0,
  None = 1
};

constexpr auto chplError = ChplChapterStatus::Error;
constexpr auto chplFound = ChplChapterStatus::Found;
constexpr auto chplNone = ChplChapterStatus::None;

struct ChplChapterResult
{
  ChplChapterStatus status{ChplChapterStatus::None};
  std::optional<int64_t> value; // For atom size
  std::optional<std::string> errorMessage; // Error messages are only returned for chplError

  constexpr ChplChapterResult() = default;

  constexpr ChplChapterResult(ChplChapterStatus s) : status{s} {}

  ChplChapterResult(ChplChapterStatus s, int64_t val) : status{s}, value{val} {}

  ChplChapterResult(ChplChapterStatus s, std::string msg) : status{s}, errorMessage{std::move(msg)}
  {
  }

  ChplChapterResult(ChplChapterStatus s, int64_t val, std::string msg)
    : status{s}, value{val}, errorMessage{std::move(msg)}
  {
  }

  explicit operator bool() const { return status == ChplChapterStatus::Found; }

  bool IsError() const { return status == ChplChapterStatus::Error; }
  bool IsFound() const { return status == ChplChapterStatus::Found; }
  bool IsNone() const { return status == ChplChapterStatus::None; }
};

class CChplChapterReader
{
public:
  /**
 * @brief Parses MP4 chapter atoms (chpl) from Nero/Ahead encoded files
 *
 * MP4 Atom Structure:
 * - All atoms start with 4-byte size (big-endian) + 4-byte tag
 * - If size == 1, next 8 bytes contain 64-bit size (big-endian)
 * - Chapter atoms are typically nested: moov -> udta -> chpl
 *
 * CHPL Atom Format:
 * - 1 byte: version (0 or 1)
 * - 3 bytes: flags (currently unused)
 * - Version 0: 1 byte chapter count
 * - Version 1: 4 bytes time_base (big-endian) + 2 bytes count (little-endian)
 * - For each chapter: 7 bytes timestamp + 1 byte title length + title string + 1 byte null
 *   terminator
 *
 * Notes:
 * - Chapter timestamp is in milliseconds for version 0, microseconds for version 1 unless there is
 *   a value in the timebase in which case it will override the default values
 * - Chapter names are utf-8 null-terminated strings
 * - Total payload size must match atom size - 8 (header bytes)
 *
 * \param url [in] url of the file to be scanned
 * \param chapters [out] vector containing the names and start times of any chapters found
 * \return 1 if no chapters found, 0 on success, -1 on error, optionally returns an error message
   */

  static ChplChapterResult ScanNeroChapters(const CURL& url, std::vector<ChplChapter>& chapters);
};
