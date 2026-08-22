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
#include "test/TestUtils.h"

#include <memory>

#include <gtest/gtest.h>

/* A container whose index ends before the physical file does - an mp4 with its
 * moov after the mdat, the default layout - reaches its true end with the read
 * position still short of the file length, and without a final zero-length
 * read to latch the input's eof flag. That end must surface promptly: only an
 * io-reported failure is a stall, and a clean end must not be polled as one.
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
