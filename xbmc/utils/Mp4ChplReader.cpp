/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Mp4ChplReader.h"

#include "URL.h"
#include "filesystem/File.h"
#include "utils/log.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include <fcntl.h>
#include <sys/types.h>

using namespace XFILE;

namespace
{
constexpr size_t ATOM_HEADER_SIZE = 8;
constexpr size_t EXTENDED_HEADER_SIZE = 16;
constexpr uint32_t V0_TIME_BASE = 1000; // timestamps will be in milliseconds
constexpr uint32_t V1_TIME_BASE = 10000; // timestamps will be in microseconds

std::string_view AtomTagToString(const std::array<char, 5>& tag)
{
  return std::string_view(tag.data(), strnlen(tag.data(), 4));
}

ChplChapterResult ErrorResult(std::string_view msg)
{
  return {ChplChapterStatus::Error, std::string{msg}};
}

constexpr std::array<char, 5> TagToString(uint32_t tag)
{
  return {static_cast<char>((tag >> 24) & 0xFF), static_cast<char>((tag >> 16) & 0xFF),
          static_cast<char>((tag >> 8) & 0xFF), static_cast<char>(tag & 0xFF), '\0'};
}

template<typename T>
constexpr T ReadBigEndian(std::span<const uint8_t> bytes)
{
  static_assert(std::is_unsigned_v<T>, "T must be an unsigned integer type");

  T res = 0;
  for (uint8_t byte : bytes)
  {
    res = (res << 8) | byte;
  }
  return res;
}

constexpr uint32_t Mkbetag(char a, char b, char c, char d)
{
  return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) |
         (static_cast<uint32_t>(c) << 8) | static_cast<uint32_t>(d);
}

// Structure to hold atom information
struct AtomInfo
{
  uint32_t tag = 0;
  int64_t size = 0;
  int64_t start_pos = 0;
  int64_t end_pos = 0;
  int header_len = 0;
};

ChplChapterResult ParseChpl(CFile& file, const int64_t chpl_size, std::vector<ChplChapter>& out)
{
  // --- header ---
  uint8_t version = 0;
  if (file.Read(&version, 1) != 1)
    return ErrorResult("Failed to read chpl version");

  std::array<uint8_t, 3> flag_buf{};
  if (file.Read(flag_buf.data(), flag_buf.size()) != static_cast<ssize_t>(flag_buf.size()))
    return ErrorResult("Failed to read chpl flags");
  const uint32_t flags =
      (uint32_t(flag_buf[0]) << 16) | (uint32_t(flag_buf[1]) << 8) | uint32_t(flag_buf[2]);

  uint32_t time_base = V0_TIME_BASE;
  uint32_t count = 0;

  if (version == 0)
  {
    uint8_t tmp;
    if (file.Read(&tmp, 1) != 1)
      return ErrorResult("Failed to read No. of chapters in chpl version 0");
    count = tmp;
  }
  else if (version == 1)
  {
    std::array<uint8_t, 4> tb_buf{};
    if (file.Read(tb_buf.data(), tb_buf.size()) != static_cast<ssize_t>(tb_buf.size()))
      return ErrorResult("Failed to read time_base");
    time_base = ReadBigEndian<uint32_t>(tb_buf);

    std::array<uint8_t, 2> cnt_buf{};
    if (file.Read(cnt_buf.data(), cnt_buf.size()) != static_cast<ssize_t>(cnt_buf.size()))
      return ErrorResult("Failed to read No. of Chapters in chpl version 1");
    count = (uint32_t(cnt_buf[1]) << 8) | cnt_buf[0];
  }
  else
  {
    const int64_t skip = chpl_size - ATOM_HEADER_SIZE - 4; // 4 is version + flags
    if (skip <= 0 || file.Seek(skip, SEEK_CUR) != skip)
      return ErrorResult("Failed to skip unkown chpl version");
    return chplNone;
  }

  CLog::LogF(LOGDEBUG, "Found chpl atom: version={}, flags={}, containing {} chapters", version,
             flags, count);

  for (uint32_t i = 0; i < count; ++i)
  { // Note: Spec claims 8 bytes, but actual implementations use 7 bytes
    // Using 8 bytes causes progressive data corruption in chapter titles
    std::array<uint8_t, 7> ts_buf{};
    if (file.Read(ts_buf.data(), ts_buf.size()) != static_cast<ssize_t>(ts_buf.size()))
      return ErrorResult("Failed to read timestamp of title");
    const uint64_t ts = ReadBigEndian<uint64_t>(ts_buf);

    uint8_t len = 0;
    if (file.Read(&len, 1) != 1)
      return ErrorResult("Failed to read chapter title length");

    std::string title(len, '\0');
    if (file.Read(title.data(), len) != len)
      return ErrorResult("Failed to read chapter title");

    if (time_base == 0)
      time_base = V1_TIME_BASE;

    ChplChapter chap{.start_pts_ms = static_cast<uint64_t>(ts) / time_base, // milliseconds
                     .title = std::move(title)};
    out.emplace_back(std::move(chap));
    // skip null terminator at end of string, we already have the data
    if (file.Read(&len, 1) != 1)
      return ErrorResult("Failed to move to next chapter timestamp");
  }
  return chplFound;
}

// Read and parse a single atom header
ChplChapterResult ReadAtomInfo(XFILE::CFile& file, AtomInfo& atom_info)
{
  atom_info.start_pos = file.GetPosition();

  std::array<uint8_t, 4> size_bytes{};
  if (file.Read(size_bytes.data(), size_bytes.size()) != static_cast<ssize_t>(size_bytes.size()))
  {
    return ErrorResult("Failed to read atom header size");
  }

  const uint32_t size32 = ReadBigEndian<uint32_t>(size_bytes);

  std::array<uint8_t, 4> tag_bytes{};
  if (file.Read(tag_bytes.data(), tag_bytes.size()) != static_cast<ssize_t>(tag_bytes.size()))
  {
    return ErrorResult("Failed to read atom tag name");
  }

  atom_info.tag = ReadBigEndian<uint32_t>(tag_bytes);
  atom_info.header_len = ATOM_HEADER_SIZE;

  if (size32 == 1)
  {
    // "largesize" - read 64-bit size
    std::array<uint8_t, 8> size64_bytes{};
    if (file.Read(size64_bytes.data(), size64_bytes.size()) !=
        static_cast<ssize_t>(size64_bytes.size()))
    {
      return ErrorResult("Failed to read atom length");
    }

    const uint64_t size64 = ReadBigEndian<uint64_t>(size64_bytes);
    atom_info.size = static_cast<int64_t>(size64);
    atom_info.header_len = EXTENDED_HEADER_SIZE;
  }
  else
  {
    atom_info.size = static_cast<int64_t>(size32);
  }

  if (atom_info.size < atom_info.header_len)
  {
    return ErrorResult("Invalid atom size");
  }

  atom_info.end_pos = atom_info.start_pos + atom_info.size;
  return chplFound;
}

// Skip an atom's payload
bool SkipAtomPayload(XFILE::CFile& file, const AtomInfo& atom_info)
{
  const int64_t skip_result = file.Seek(atom_info.size - atom_info.header_len, SEEK_CUR);
  const int64_t expected_pos = atom_info.end_pos;

  if (skip_result != expected_pos)
  {
    const auto tag_str = TagToString(atom_info.tag);
    CLog::LogF(LOGWARNING, "Skip mismatch at atom {} - expected {}, got {}",
               AtomTagToString(tag_str), expected_pos, skip_result);
    file.Seek(expected_pos, SEEK_SET);
  }
  return true;
}

ChplChapterResult ParseAtomChildren(XFILE::CFile& file,
                                    const AtomInfo& parent_atom,
                                    std::vector<ChplChapter>& chapters);

// Process a single atom
ChplChapterResult ProcessAtom(XFILE::CFile& file,
                              const AtomInfo& atom_info,
                              std::vector<ChplChapter>& chapters)
{
  switch (atom_info.tag)
  {
    case Mkbetag('c', 'h', 'p', 'l'):
      // Found chpl atom - parse it
      return ParseChpl(file, atom_info.size, chapters);

    case Mkbetag('m', 'o', 'o', 'v'):
    case Mkbetag('u', 'd', 't', 'a'):
      // Container atoms - parse their children
      return ParseAtomChildren(file, atom_info, chapters);

    default:
      // Skip unknown atoms
      SkipAtomPayload(file, atom_info);
      return chplNone;
  }
}

// Parse all children of a container atom
ChplChapterResult ParseAtomChildren(XFILE::CFile& file,
                                    const AtomInfo& parent_atom,
                                    std::vector<ChplChapter>& chapters)
{
  while (file.GetPosition() < parent_atom.end_pos)
  {
    AtomInfo child_atom;
    auto result = ReadAtomInfo(file, child_atom);
    if (result.IsError())
    {
      return result;
    }

    result = ProcessAtom(file, child_atom, chapters);
    if (result.IsError())
    {
      return result;
    }

    // Ensure we're at the end of this child atom
    if (file.GetPosition() != child_atom.end_pos)
    {
      file.Seek(child_atom.end_pos, SEEK_SET);
    }
  }
  return chplNone;
}

} // unnamed namespace

ChplChapterResult CChplChapterReader::ScanNeroChapters(const CURL& url,
                                                       std::vector<ChplChapter>& chapters)
{
  CFile file;
  if (!file.Open(url, READ_TRUNCATED))
  {
    return ErrorResult("Unable to open input file");
  }

  const int64_t file_size = file.GetLength();

  if (file.Seek(0, SEEK_SET) != 0)
  {
    file.Close();
    return ErrorResult("Unable to seek to beginning of file");
  }

  // Parse all top-level atoms
  while (file.GetPosition() < file_size)
  {
    AtomInfo atom_info;
    auto result = ReadAtomInfo(file, atom_info);
    if (result.IsError())
    {
      file.Close();
      return result;
    }

    result = ProcessAtom(file, atom_info, chapters);
    if (result.IsError())
    {
      file.Close();
      return result;
    }

    // Ensure we're at the end of this atom
    if (file.GetPosition() != atom_info.end_pos)
    {
      file.Seek(atom_info.end_pos, SEEK_SET);
    }
  }

  file.Close();
  return chapters.empty() ? chplNone : chplFound;
}
