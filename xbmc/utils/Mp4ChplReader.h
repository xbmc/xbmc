/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
// The chpl atom is used for Nero chapters, which are an alternative to Quicktime chapters
// implemented by Nero software suite in 2005 in mp4 and related files

#pragma once

#include "filesystem/File.h"

#include <source_location>

extern "C" {
#include <libavutil/avconfig.h>
}

struct ChplChapter
{
  uint64_t start_pts; // milliseconds
  std::string title;
};

enum class ChplChapterResult : int
{
  Error = -1,
  Found = 0,
  None = 1
};

constexpr auto chplError = ChplChapterResult::Error;
constexpr auto chplFound = ChplChapterResult::Found;
constexpr auto chplNone = ChplChapterResult::None;

using namespace XFILE;

class CChplChapterReader
{
public:
  /*! \brief Scan a file for the chpl atom and if present, read the chapter names it contains
    \param url [in] url of the file to be scanned
    \param chapters [out] vector containing the names and start times of any chapters found
    \return 1 if no chapters found, 0 on success, -1 on error
   */
  static ChplChapterResult scan_nero_chapters(const CURL& url, std::vector<ChplChapter>& chapters);

private:
  static int64_t read_atom_header(CFile& file, uint32_t& tag, int& hdr_len);
  static ChplChapterResult parse_chpl(CFile& file,
                                      const int64_t chpl_size,
                                      std::vector<ChplChapter>& out);
};
