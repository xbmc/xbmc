/*
 *  Copyright (C) 2025 Team Kodi
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
constexpr size_t CHPL_VERSION_FLAGS_SIZE = 4; // 1 byte for version, 3 bytes for flags (unused)
constexpr auto V1_TIME_BASE = std::chrono::nanoseconds(100); // 100ns units

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
  int64_t startPos = 0;
  int64_t endPos = 0;
  int headerLength = 0;
};

ChplChapterResult ParseChpl(CFile& file, const int64_t chplSize, std::vector<ChplChapter>& out)
{
  // --- header ---
  uint8_t version = 0;
  if (file.Read(&version, 1) != 1)
    return ErrorResult("Failed to read chpl version");

  std::array<uint8_t, 3> flagBuf{};
  if (file.Read(flagBuf.data(), flagBuf.size()) != static_cast<ssize_t>(flagBuf.size()))
    return ErrorResult("Failed to read chpl flags");
  const uint32_t flags =
      (uint32_t(flagBuf[0]) << 16) | (uint32_t(flagBuf[1]) << 8) | uint32_t(flagBuf[2]);

  uint32_t count = 0;
  std::chrono::nanoseconds timeBase;

  if (version == 0)
  {
    uint8_t tmp;
    if (file.Read(&tmp, 1) != 1)
      return ErrorResult("Failed to read No. of chapters in chpl version 0");
    count = tmp;
  }
  else if (version == 1)
  {
    std::array<uint8_t, 4> timeBaseBuf{};
    if (file.Read(timeBaseBuf.data(), timeBaseBuf.size()) !=
        static_cast<ssize_t>(timeBaseBuf.size()))
      return ErrorResult("Failed to read time base");
    const uint32_t tbRaw = ReadBigEndian<uint32_t>(timeBaseBuf);
    timeBase = (tbRaw != 0) ? std::chrono::nanoseconds(tbRaw) : V1_TIME_BASE;

    std::array<uint8_t, 2> countBuf{};
    if (file.Read(countBuf.data(), countBuf.size()) != static_cast<ssize_t>(countBuf.size()))
      return ErrorResult("Failed to read No. of Chapters in chpl version 1");
    count = (uint32_t(countBuf[1]) << 8) | countBuf[0];
  }
  else
  {
    const int64_t skip = chplSize - ATOM_HEADER_SIZE - CHPL_VERSION_FLAGS_SIZE;
    if (skip <= 0 || file.Seek(skip, SEEK_CUR) != skip)
      return ErrorResult("Failed to skip unknown chpl version");
    return chplNone;
  }

  CLog::LogF(LOGDEBUG, "Found chpl atom: version={}, flags={}, containing {} chapters", version,
             flags, count);

  for (uint32_t i = 0; i < count; ++i)
  { // Note: Spec claims 8 bytes, but actual implementations use 7 bytes
    // Using 8 bytes will cause progressive data corruption in chapter titles and incorrect times
    // See the example hex in xbmc/utils/Mp4ChplReader.h and breakdown for more information
    std::array<uint8_t, 7> timestampBuf{};
    if (file.Read(timestampBuf.data(), timestampBuf.size()) !=
        static_cast<ssize_t>(timestampBuf.size()))
    {
      return ErrorResult("Failed to read timestamp of title");
    }
    const uint64_t ts = ReadBigEndian<uint64_t>(timestampBuf);

    uint8_t len = 0;
    if (file.Read(&len, 1) != 1)
      return ErrorResult("Failed to read chapter title length");

    std::string title(len, '\0');
    if (file.Read(title.data(), len) != len)
      return ErrorResult("Failed to read chapter title");

    std::chrono::nanoseconds timeNs{};
    if (version == 0)
      timeNs = std::chrono::milliseconds(ts);
    else
      timeNs = timeBase * ts;
    ChplChapter chap{.startPts = std::chrono::duration_cast<std::chrono::milliseconds>(timeNs),
                     .title = std::move(title)};
    out.emplace_back(std::move(chap));

    // skip null terminator at end of string, we already have the data
    if (file.Read(&len, 1) != 1)
      return ErrorResult("Failed to move to next chapter timestamp");
  }
  return chplFound;
}

// Read and parse a single atom header
ChplChapterResult ReadAtomInfo(XFILE::CFile& file, AtomInfo& atomInfo)
{
  atomInfo.startPos = file.GetPosition();

  std::array<uint8_t, 4> sizeBytes{};
  if (file.Read(sizeBytes.data(), sizeBytes.size()) != static_cast<ssize_t>(sizeBytes.size()))
  {
    return ErrorResult("Failed to read atom header size");
  }

  const uint32_t size32 = ReadBigEndian<uint32_t>(sizeBytes);

  std::array<uint8_t, 4> tagBytes{};
  if (file.Read(tagBytes.data(), tagBytes.size()) != static_cast<ssize_t>(tagBytes.size()))
  {
    return ErrorResult("Failed to read atom tag name");
  }

  atomInfo.tag = ReadBigEndian<uint32_t>(tagBytes);
  atomInfo.headerLength = ATOM_HEADER_SIZE;

  if (size32 == 1)
  {
    // "largesize" - read 64-bit size
    std::array<uint8_t, 8> size64Bytes{};
    if (file.Read(size64Bytes.data(), size64Bytes.size()) !=
        static_cast<ssize_t>(size64Bytes.size()))
    {
      return ErrorResult("Failed to read atom length");
    }

    const uint64_t size64 = ReadBigEndian<uint64_t>(size64Bytes);
    atomInfo.size = static_cast<int64_t>(size64);
    atomInfo.headerLength = EXTENDED_HEADER_SIZE;
  }
  else
  {
    atomInfo.size = static_cast<int64_t>(size32);
  }

  if (atomInfo.size < atomInfo.headerLength)
  {
    return ErrorResult("Invalid atom size");
  }

  atomInfo.endPos = atomInfo.startPos + atomInfo.size;
  return chplFound;
}

// Skip an atom's payload
bool SkipAtomPayload(XFILE::CFile& file, const AtomInfo& atomInfo)
{
  const int64_t skipResult = file.Seek(atomInfo.size - atomInfo.headerLength, SEEK_CUR);
  const int64_t expectedPos = atomInfo.endPos;

  if (skipResult != expectedPos)
  {
    const auto tagString = TagToString(atomInfo.tag);
    CLog::LogF(LOGWARNING, "Skip mismatch at atom {} - expected {}, got {}",
               AtomTagToString(tagString), expectedPos, skipResult);
    file.Seek(expectedPos, SEEK_SET);
  }
  return true;
}

ChplChapterResult ParseAtomChildren(XFILE::CFile& file,
                                    const AtomInfo& parentAtom,
                                    std::vector<ChplChapter>& chapters);

// Process a single atom
ChplChapterResult ProcessAtom(XFILE::CFile& file,
                              const AtomInfo& atomInfo,
                              std::vector<ChplChapter>& chapters)
{
  switch (atomInfo.tag)
  {
    case Mkbetag('c', 'h', 'p', 'l'):
      // Found chpl atom - parse it
      return ParseChpl(file, atomInfo.size, chapters);

    case Mkbetag('m', 'o', 'o', 'v'):
    case Mkbetag('u', 'd', 't', 'a'):
      // Container atoms - parse their children
      return ParseAtomChildren(file, atomInfo, chapters);

    default:
      // Skip unknown atoms
      SkipAtomPayload(file, atomInfo);
      return chplNone;
  }
}

// Parse all children of a container atom
ChplChapterResult ParseAtomChildren(XFILE::CFile& file,
                                    const AtomInfo& parentAtom,
                                    std::vector<ChplChapter>& chapters)
{
  while (file.GetPosition() < parentAtom.endPos)
  {
    AtomInfo childAtom;
    auto result = ReadAtomInfo(file, childAtom);
    if (result.IsError())
    {
      return result;
    }

    result = ProcessAtom(file, childAtom, chapters);
    if (result.IsError())
    {
      return result;
    }

    // Ensure we're at the end of this child atom
    if (file.GetPosition() != childAtom.endPos)
    {
      file.Seek(childAtom.endPos, SEEK_SET);
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

  const int64_t fileSize = file.GetLength();

  if (file.Seek(0, SEEK_SET) != 0)
  {
    file.Close();
    return ErrorResult("Unable to seek to beginning of file");
  }

  // Parse all top-level atoms
  while (file.GetPosition() < fileSize)
  {
    AtomInfo atomInfo;
    auto result = ReadAtomInfo(file, atomInfo);
    if (result.IsError())
    {
      file.Close();
      return result;
    }

    result = ProcessAtom(file, atomInfo, chapters);
    if (result.IsError())
    {
      file.Close();
      return result;
    }

    // Ensure we're at the end of this atom
    if (file.GetPosition() != atomInfo.endPos)
    {
      file.Seek(atomInfo.endPos, SEEK_SET);
    }
  }

  file.Close();
  return chapters.empty() ? chplNone : chplFound;
}
