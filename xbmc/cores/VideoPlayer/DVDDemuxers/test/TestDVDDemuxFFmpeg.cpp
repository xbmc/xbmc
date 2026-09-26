/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStreamFile.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "filesystem/File.h"
#include "test/TestUtils.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
/* Serves a real file from memory and, once told to stall, behaves like the file cache over a
 * frozen network flow: each read blocks and then reports an error.
 */
class CStallingInputStream : public CDVDInputStreamFile
{
public:
  explicit CStallingInputStream(const CFileItem& fileitem) : CDVDInputStreamFile(fileitem, 0) {}

  bool Open() override
  {
    XFILE::CFile file;
    if (!file.Open(m_item.GetPath()))
      return false;
    const int64_t length = file.GetLength();
    if (length <= 0)
      return false;
    m_data.resize(static_cast<size_t>(length));
    int64_t total = 0;
    while (total < length)
    {
      const ssize_t read = file.Read(m_data.data() + total, length - total);
      if (read <= 0)
        return false;
      total += read;
    }
    m_pos = 0;
    return true;
  }

  void Close() override {}

  int Read(uint8_t* buf, int buf_size) override
  {
    if (m_stalled.load())
    {
      // Longer than the demuxer's read timer, so the timer fires before this failure surfaces
      for (int i = 0; i < 20 && m_stalled.load(); ++i)
        std::this_thread::sleep_for(100ms);
      if (m_stalled.load())
        return -1;
    }
    if (m_pos >= static_cast<int64_t>(m_data.size()))
    {
      m_eofLatched = true;
      return 0;
    }
    const int count =
        static_cast<int>(std::min<int64_t>(buf_size, static_cast<int64_t>(m_data.size()) - m_pos));
    std::memcpy(buf, m_data.data() + m_pos, count);
    m_pos += count;
    return count;
  }

  int64_t Seek(int64_t offset, int whence) override
  {
    if (whence == DVDSTREAM_SEEK_POSSIBLE)
      return 1;
    int64_t target = -1;
    if (whence == SEEK_SET)
      target = offset;
    else if (whence == SEEK_CUR)
      target = m_pos + offset;
    else if (whence == SEEK_END)
      target = static_cast<int64_t>(m_data.size()) + offset;
    if (target < 0 || target > static_cast<int64_t>(m_data.size()))
      return -1;
    m_pos = target;
    m_eofLatched = false;
    return m_pos;
  }

  bool IsEOF() override { return m_eofLatched; }
  int64_t GetLength() override { return static_cast<int64_t>(m_data.size()); }
  int GetBlockSize() override { return 0; }

  void Stall(bool stalled) { m_stalled = stalled; }

private:
  std::vector<uint8_t> m_data;
  int64_t m_pos = 0;
  bool m_eofLatched = false;
  std::atomic<bool> m_stalled{false};
};
} // namespace

/* A container whose index ends before the physical file does - an mp4 with its
 * moov after the mdat, the default layout - reaches its true end with the read
 * position still short of the file length, and without a final zero-length
 * read to latch the input's eof flag. That end must surface promptly.
 */
TEST(TestDVDDemuxFFmpeg, IndexedEofWithTrailingBytesIsPrompt)
{
  const std::string path = XBMC_REF_FILE_PATH("xbmc/utils/test/resources/no_chapters.mp4");
  const CFileItem item(path, false);
  auto input = std::make_shared<CDVDInputStreamFile>(item, 0);
  ASSERT_TRUE(input->Open());

  CDVDDemuxFFmpeg demux;
  ASSERT_TRUE(demux.Open(input, false));

  int dataPackets = 0;
  int emptyPackets = 0;
  while (DemuxPacket* packet = demux.Read())
  {
    if (packet->iSize > 0)
      ++dataPackets;
    else
      ++emptyPackets;
    CDVDDemuxUtils::FreeDemuxPacket(packet);
    if (emptyPackets > 32)
      break;
  }

  EXPECT_GT(dataPackets, 0) << "the fixture demuxed no data at all";
  EXPECT_LE(emptyPackets, 32) << "a cleanly ended file was polled as a stalled one";
}

/* A source that freezes mid-file blocks inside av_read_frame, so the demuxer's
 * own read timer fires before any io error can surface.
 *
 * The matroska fixture is generated:
 *
 *   ffmpeg -f lavfi -i "smptebars=s=320x240:r=10:d=2" \
 *          -c:v libx264 -preset veryslow -crf 30 -pix_fmt yuv420p -g 10 \
 *          h264_8bit_320x240.mkv
 */
TEST(TestDVDDemuxFFmpeg, StalledInputOutlivesTheReadTimer)
{
  const std::string path =
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/h264_8bit_320x240.mkv");
  const CFileItem item(path, false);
  auto input = std::make_shared<CStallingInputStream>(item);
  ASSERT_TRUE(input->Open());

  CDVDDemuxFFmpeg demux;
  demux.SetReadTimeout(1s);
  demux.SetStallRecoveryWindow(60s);
  ASSERT_TRUE(demux.Open(input, false));

  int dataPackets = 0;
  while (dataPackets < 8)
  {
    DemuxPacket* packet = demux.Read();
    ASSERT_NE(nullptr, packet) << "demuxing the healthy start of the file failed";
    if (packet->iSize > 0)
      ++dataPackets;
    CDVDDemuxUtils::FreeDemuxPacket(packet);
  }

  input->Stall(true);
  const auto stallStart = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - stallStart < 6s)
  {
    DemuxPacket* packet = demux.Read();
    const auto stalledFor = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - stallStart);
    ASSERT_NE(nullptr, packet) << "demuxer gave up " << stalledFor.count()
                               << "s into a recoverable stall";
    CDVDDemuxUtils::FreeDemuxPacket(packet);
  }
  input->Stall(false);

  int recovered = 0;
  for (int i = 0; i < 256 && recovered == 0; ++i)
  {
    DemuxPacket* packet = demux.Read();
    ASSERT_NE(nullptr, packet) << "demuxer did not survive the stall clearing";
    if (packet->iSize > 0)
      ++recovered;
    CDVDDemuxUtils::FreeDemuxPacket(packet);
  }
  EXPECT_GT(recovered, 0) << "no data packet arrived after the input recovered";
}
