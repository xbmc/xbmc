/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IndexParser.h"

#include "BitReader.h"
#include "URL.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace XFILE
{
namespace
{
constexpr std::string_view INDX_HEADER = "INDX";
constexpr unsigned int HEADER_SIZE = 12; // magic, version and indexes start address
constexpr unsigned int INDEXES_START_OFFSET = 8;

// Each index entry is 12 bytes - 4 bytes of flags followed by an 8 byte object reference
constexpr unsigned int INDEX_ENTRY_SIZE = 12;
constexpr unsigned int OFFSET_ENTRY_OBJECT = 4;
constexpr unsigned int OFFSET_HDMV_MOVIE_OBJECT = 2;
constexpr unsigned int OFFSET_BDJ_NAME = 2;
constexpr unsigned int BDJ_NAME_LENGTH = 5;

// index.bdmv is a small table of contents - anything larger is not one
constexpr int64_t MAX_FILE_SIZE = 64 * 1024;

/*!
 \brief Parse one 12 byte index entry.
 \param isTitle title entries carry an access type in the flags where the first playback and top
        menu entries have only the object type.
 */
IndexObjectInformation ParseIndexEntry(const std::span<const std::byte> buffer,
                                       unsigned int offset,
                                       bool isTitle)
{
  IndexObjectInformation information;

  const uint8_t flags{GetByte(buffer, offset)};
  information.objectType = static_cast<BLURAY_OBJECT_TYPE>((flags >> 6) & 0x03);
  if (isTitle)
    information.accessType = (flags >> 4) & 0x03;

  const unsigned int object{offset + OFFSET_ENTRY_OBJECT};
  switch (information.objectType)
  {
    case BLURAY_OBJECT_TYPE::HDMV:
      information.playbackType =
          static_cast<BLURAY_TITLE_PLAYBACK_TYPE>((GetByte(buffer, object) >> 6) & 0x03);
      information.movieObject = GetWord(buffer, object + OFFSET_HDMV_MOVIE_OBJECT);
      break;

    case BLURAY_OBJECT_TYPE::BDJ:
      information.playbackType =
          static_cast<BLURAY_TITLE_PLAYBACK_TYPE>((GetByte(buffer, object) >> 6) & 0x03);
      information.bdjObject = GetString(buffer, object + OFFSET_BDJ_NAME, BDJ_NAME_LENGTH);
      break;

    default:
      break;
  }

  return information;
}

bool ParseIndex(const std::span<const std::byte> buffer, IndexInformation& indexInformation)
{
  if (buffer.size() < HEADER_SIZE ||
      !std::equal(INDX_HEADER.begin(), INDX_HEADER.end(),
                  reinterpret_cast<const char*>(buffer.data())))
  {
   CLog::LogF(LOGDEBUG, "Invalid index.bdmv header");
    return false;
  }

  indexInformation.version = GetString(buffer, INDX_HEADER.size(), 4);

  // The first playback and top menu entries are followed by the title count and the titles
  unsigned int offset{GetDWord(buffer, INDEXES_START_OFFSET)};
  offset += 4; // length of the indexes section

  indexInformation.firstPlayback = ParseIndexEntry(buffer, offset, false);
  offset += INDEX_ENTRY_SIZE;
  indexInformation.topMenu = ParseIndexEntry(buffer, offset, false);
  offset += INDEX_ENTRY_SIZE;

  const uint16_t numberOfTitles{GetWord(buffer, offset)};
  offset += 2;

  if (buffer.size() < static_cast<uint64_t>(offset) +
                          static_cast<uint64_t>(numberOfTitles) * INDEX_ENTRY_SIZE)
  {
   CLog::LogF(LOGDEBUG, "Truncated index.bdmv - {} titles do not fit",
                numberOfTitles);
    return false;
  }

  indexInformation.titles.reserve(numberOfTitles);
  for (uint16_t i = 0; i < numberOfTitles; ++i, offset += INDEX_ENTRY_SIZE)
    indexInformation.titles.emplace_back(ParseIndexEntry(buffer, offset, true));

  return true;
}
} // namespace

bool IndexInformation::HasHdmvObjects() const
{
  return firstPlayback.objectType == BLURAY_OBJECT_TYPE::HDMV ||
         topMenu.objectType == BLURAY_OBJECT_TYPE::HDMV ||
         std::ranges::any_of(titles, [](const IndexObjectInformation& title)
                             { return title.objectType == BLURAY_OBJECT_TYPE::HDMV; });
}

bool IndexInformation::HasHdmvTopMenu() const
{
  return topMenu.objectType == BLURAY_OBJECT_TYPE::HDMV &&
         topMenu.movieObject != MOVIE_OBJECT_NONE;
}

MovieObjectUsage IndexInformation::GetMovieObjectUsage(unsigned int movieObject) const
{
  const auto isObject = [movieObject](const IndexObjectInformation& information)
  {
    return information.objectType == BLURAY_OBJECT_TYPE::HDMV &&
           information.movieObject == movieObject;
  };

  MovieObjectUsage usage;
  usage.firstPlayback = isObject(firstPlayback);
  usage.topMenu = isObject(topMenu);
  for (unsigned int i = 0; i < titles.size(); ++i)
  {
    if (isObject(titles[i]))
      usage.titles.emplace_back(i + 1); // titles are numbered from 1
  }
  return usage;
}

bool CIndexParser::ReadIndex(const CURL& url, IndexInformation& indexInformation)
{
  const std::string indexFile{URIUtils::AddFileToFolder(url.GetHostName(), "BDMV", "index.bdmv")};

  CFile file;
  if (!file.Open(indexFile))
  {
   CLog::LogF(LOGDEBUG, "Could not open index.bdmv");
    return false;
  }

  const int64_t size{file.GetLength()};
  if (size < HEADER_SIZE || size > MAX_FILE_SIZE)
  {
   CLog::LogF(LOGDEBUG, "Invalid index.bdmv size {}", size);
    return false;
  }

  std::vector<std::byte> buffer(static_cast<size_t>(size));
  size_t total{0};
  while (total < buffer.size())
  {
    const ssize_t read{file.Read(buffer.data() + total, buffer.size() - total)};
    if (read <= 0)
      break;
    total += static_cast<size_t>(read);
  }

  if (total != buffer.size())
  {
   CLog::LogF(LOGDEBUG, "Could not read index.bdmv");
    return false;
  }

  try
  {
    return ParseIndex(buffer, indexInformation);
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "index.bdmv parsing failed - error {}", e.what());
    return false;
  }
}
} // namespace XFILE
