/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Mp4ChplReader.h"

#include "URL.h"
#include "cores/FFmpeg.h"
#include "utils/log.h"

using namespace XFILE;

ChplChapterResult CChplChapterReader::scan_nero_chapters(const CURL& url,
                                                         std::vector<ChplChapter>& chapters)
{
  const auto logLocation = std::source_location::current();

  CFile file;
  if (!file.Open(url, READ_TRUNCATED))
  {
    CLog::Log(LOGERROR, "{}: Cannot open {}", logLocation.function_name(), url.GetRedacted());
    return chplError;
  }

  // Get file size
  const int64_t file_size = file.GetLength();

  // Seek to beginning
  const int64_t seek_result = file.Seek(0, SEEK_SET);
  if (seek_result != 0)
  {
    CLog::Log(LOGERROR, "{}: Cannot seek to beginning of file {}", logLocation.function_name(),
              url.GetRedacted());
    file.Close();
    return chplError;
  }

  // Parse MP4 atoms
  while (file.GetPosition() < file_size)
  {
    int64_t atom_start = file.GetPosition();

    uint32_t tag;
    int hdr_len = 0;
    const int64_t size = read_atom_header(file, tag, hdr_len);

    if (size < hdr_len)
    {
      CLog::Log(LOGERROR, "{}: Invalid atom size at position {}", logLocation.function_name(),
                atom_start);
      file.Close();
      return chplError;
    }

    const int64_t atom_end = atom_start + size;

    char tag_str[5] = {0};
    tag_str[0] = (tag >> 24) & 0xFF;
    tag_str[1] = (tag >> 16) & 0xFF;
    tag_str[2] = (tag >> 8) & 0xFF;
    tag_str[3] = tag & 0xFF;

    switch (tag)
    {
      case MKBETAG('m', 'o', 'o', 'v'):
      case MKBETAG('u', 'd', 't', 'a'):
        while (file.GetPosition() < atom_end)
        {
          const int64_t child_start = file.GetPosition();
          uint32_t child_tag;
          int child_hdr_len = 0;
          const int64_t child_size = read_atom_header(file, child_tag, child_hdr_len);

          if (child_size < child_hdr_len)
          {
            CLog::Log(LOGERROR, "{}: Invalid child atom size at position {}",
                      logLocation.function_name(), child_start);
            file.Close();
            return chplError;
          }

          char child_tag_str[5] = {0};
          child_tag_str[0] = (child_tag >> 24) & 0xFF;
          child_tag_str[1] = (child_tag >> 16) & 0xFF;
          child_tag_str[2] = (child_tag >> 8) & 0xFF;
          child_tag_str[3] = child_tag & 0xFF;

          if (child_tag == MKBETAG('c', 'h', 'p', 'l'))
          {
            // Position is right after header; parse payload
            const ChplChapterResult ret = parse_chpl(file, child_size, chapters);
            if (ret == chplError)
            {
              CLog::Log(LOGERROR, "{}:  Error parsing chpl atom in file {}",
                        logLocation.function_name(), url.GetRedacted());
              file.Close();
              return ret;
            }
          }
          else if (child_tag == MKBETAG('u', 'd', 't', 'a'))
          {
            // udta can contain chpl - parse its children
            const int64_t udta_end = child_start + child_size;
            while (file.GetPosition() < udta_end)
            {
              const int64_t grandchild_start = file.GetPosition();
              uint32_t grandchild_tag;
              int grandchild_hdr_len = 0;
              const int64_t grandchild_size =
                  read_atom_header(file, grandchild_tag, grandchild_hdr_len);

              if (grandchild_size < grandchild_hdr_len)
              {
                CLog::Log(LOGERROR, "{}: Invalid grandchild atom size at position {}",
                          logLocation.function_name(), grandchild_start);
                file.Close();
                return chplError;
              }

              char grandchild_tag_str[5] = {0};
              grandchild_tag_str[0] = (grandchild_tag >> 24) & 0xFF;
              grandchild_tag_str[1] = (grandchild_tag >> 16) & 0xFF;
              grandchild_tag_str[2] = (grandchild_tag >> 8) & 0xFF;
              grandchild_tag_str[3] = grandchild_tag & 0xFF;

              if (grandchild_tag == MKBETAG('c', 'h', 'p', 'l'))
              {
                // Found chpl inside udta - parse it
                const ChplChapterResult ret = parse_chpl(file, grandchild_size, chapters);
                if (ret <= chplError)
                {
                  CLog::Log(LOGERROR, "{}:  Error parsing chpl atom in file {}",
                            logLocation.function_name(), url.GetRedacted());
                  file.Close();
                  return ret;
                }
              }
              else
              {
                // Skip this grandchild atom
                const int64_t skip_result =
                    file.Seek(grandchild_size - grandchild_hdr_len, SEEK_CUR);
                const int64_t expected_pos = grandchild_start + grandchild_size;
                if (skip_result != expected_pos)
                {
                  CLog::Log(LOGWARNING, "{}: Skip mismatch at grandchild {} - expected {}, got {}",
                            logLocation.function_name(), grandchild_tag_str, expected_pos,
                            skip_result);
                  file.Seek(expected_pos, SEEK_SET);
                }
              }
            }
          }
          else
          {
            // Skip the payload (size - hdr_len)
            const int64_t skip_result = file.Seek(child_size - child_hdr_len, SEEK_CUR);
            const int64_t expected_pos = child_start + child_size;
            if (skip_result != expected_pos)
            {
              CLog::Log(LOGWARNING, "{}: Skip mismatch at child {} - expected {}, got {}",
                        logLocation.function_name(), child_tag_str, expected_pos, skip_result);
              // Force correct position
              file.Seek(expected_pos, SEEK_SET);
            }
          }
        }
        break;

      default:
        // Atom we don't care about – skip the whole thing (size - hdr_len)
        const int64_t skip_result = file.Seek(size - hdr_len, SEEK_CUR);
        const int64_t expected_pos = atom_start + size;
        if (skip_result != expected_pos)
        {
          CLog::Log(LOGWARNING, "{}: Skip mismatch at atom {} - expected {}, got {}",
                    logLocation.function_name(), tag_str, expected_pos, skip_result);
          // Force correct position
          file.Seek(expected_pos, SEEK_SET);
        }
        break;
    }

    // Ensure we're at the end of this atom
    if (file.GetPosition() != atom_end)
    {
      file.Seek(atom_end, SEEK_SET);
    }
  }

  file.Close();
  if (chapters.size() == 0)
    return chplNone; // successful finish - no chapters found
  else
    return chplFound;
}

int64_t CChplChapterReader::read_atom_header(CFile& file, uint32_t& tag, int& hdr_len)
{
  uint8_t size_bytes[4];
  if (file.Read(size_bytes, 4) != 4)
  {
    return -1;
  }
  // Read size as big-endian
  const uint32_t size32 =
      (size_bytes[0] << 24) | (size_bytes[1] << 16) | (size_bytes[2] << 8) | size_bytes[3];

  uint8_t tag_bytes[4];
  if (file.Read(tag_bytes, 4) != 4)
  {
    return -1;
  }

  // Read tag as big-endian
  tag = (tag_bytes[0] << 24) | (tag_bytes[1] << 16) | (tag_bytes[2] << 8) | tag_bytes[3];

  hdr_len = 8;

  if (size32 == 1)
  {
    // "largesize" - read 64-bit size
    uint8_t size64_bytes[8];
    if (file.Read(size64_bytes, 8) != 8)
    {
      return -1;
    }

    // Convert 64-bit big-endian to int64_t
    uint64_t size64 = 0;
    for (int i = 0; i < 8; i++)
    {
      size64 = (size64 << 8) | size64_bytes[i];
    }

    hdr_len = 16;
    return static_cast<int64_t>(size64);
  }

  return static_cast<int64_t>(size32);
}

ChplChapterResult CChplChapterReader::parse_chpl(CFile& file,
                                                 const int64_t chpl_size,
                                                 std::vector<ChplChapter>& out)
{
  // --- header ---
  uint8_t version;
  if (file.Read(&version, 1) != 1)
    return chplError;

  uint8_t flag_buf[3];
  if (file.Read(flag_buf, 3) != 3)
    return chplError;
  const uint32_t flags = (uint32_t(flag_buf[0]) << 16) | (uint32_t(flag_buf[1]) << 8) |
                         uint32_t(flag_buf[2]); // currently unused

  uint32_t time_base = 1000; // default for v0 (milliseconds)
  uint32_t count = 0;

  if (version == 0)
  {
    uint8_t tmp;
    if (file.Read(&tmp, 1) != 1)
      return chplError;
    count = tmp;
  }
  else if (version == 1)
  {
    // read time_base – 4 bytes big‑endian
    uint8_t tb_buf[4];
    if (file.Read(tb_buf, 4) != 4)
      return chplError;
    time_base = (uint32_t(tb_buf[0]) << 24) | (uint32_t(tb_buf[1]) << 16) |
                (uint32_t(tb_buf[2]) << 8) | uint32_t(tb_buf[3]);

    // read entry_count – 2 bytes little‑endian
    uint8_t cnt_buf[2];
    if (file.Read(cnt_buf, 2) != 2)
      return chplError;
    count = (uint32_t(cnt_buf[1]) << 8) | cnt_buf[0]; // LE
  }
  else
  {
    const int64_t skip = chpl_size - 8 /*hdr*/ - 4 /*ver+flags*/;
    if (skip <= 0 || file.Seek(skip, SEEK_CUR) != skip)
      return chplError;
    return chplNone; // unknown version – safely skipped - no error
  }

  auto logLocation = std::source_location::current();

  CLog::Log(LOGDEBUG, "{}: Found chpl atom: version={}, flags={}, containing {} chapters",
            logLocation.function_name(), version, flags, count);

  for (uint32_t i = 0; i < count; ++i)
  {
    uint8_t ts_buf[7];
    if (file.Read(ts_buf, 7) != 7)
      return chplError;
    const uint64_t ts = (uint64_t(ts_buf[0]) << 48) | (uint64_t(ts_buf[1]) << 40) |
                        (uint64_t(ts_buf[2]) << 32) | (uint64_t(ts_buf[3]) << 24) |
                        (uint64_t(ts_buf[4]) << 16) | (uint64_t(ts_buf[5]) << 8) |
                        (uint64_t(ts_buf[6]));

    uint8_t len;
    if (file.Read(&len, 1) != 1)
      return chplError;

    std::string title(len, '\0');
    if (file.Read(title.data(), len) != len)
      return chplError;

    if (time_base == 0) // no timebase in file, avoid divide by zero error
      time_base = 10000; // nanoseconds

    ChplChapter chap;
    chap.start_pts =
        static_cast<uint64_t>(ts) * (AV_TIME_BASE / time_base) / 1000000; //milliseconds
    chap.title = std::move(title);

    out.push_back(std::move(chap));
    if (file.Read(&len, 1) != 1)
      return chplError;
  }
  return chplFound;
}
