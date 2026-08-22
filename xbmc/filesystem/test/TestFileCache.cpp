/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/FileCache.h"
#include "filesystem/IFileTypes.h"
#include "filesystem/PipeFile.h"

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

/* A source that merely stops answering must not surface as 0. A pipe stands in for
 * one: reads block while it is empty and the cache's fill thread parks inside the
 * read. A pipe is not seekable, so this does not exercise the reconnect path.
 */
TEST(TestFileCache, StalledSourceMidFileIsNotEof)
{
  const CURL pipeUrl("pipe://filecache-stall-test/");

  CPipeFile writer;
  ASSERT_TRUE(writer.OpenForWrite(pipeUrl));

  CFileCache cache{static_cast<unsigned int>(READ_AUDIO_VIDEO)};
  ASSERT_TRUE(cache.Open(pipeUrl));

  // The cache refreshes the source's length every fill pass; report an honest
  // size so the stall happens mid-file rather than at an unknown position.
  constexpr int64_t fileSize = 10 * 1024 * 1024;
  auto* source = static_cast<CPipeFile*>(cache.GetFileImp());
  ASSERT_NE(nullptr, source);
  source->SetLength(fileSize);

  constexpr size_t delivered = 1024 * 1024;
  const std::vector<char> data(delivered, 'x');
  ASSERT_EQ(static_cast<ssize_t>(delivered), writer.Write(data.data(), delivered));

  std::vector<char> buffer(64 * 1024);
  size_t total = 0;
  while (total < delivered)
  {
    const ssize_t read = cache.Read(buffer.data(), std::min(buffer.size(), delivered - total));
    ASSERT_GT(read, 0) << "read failed at offset " << total << " with all data still available";
    total += read;
  }

  // The source is now stalled with most of the file outstanding.
  const ssize_t starved = cache.Read(buffer.data(), buffer.size());
  EXPECT_NE(0, starved) << "a stalled source was reported as end-of-file at position "
                        << cache.GetPosition() << " of " << cache.GetLength();

  // Eof must be raised before Close: the fill thread may be blocked inside
  // the pipe read, and Close waits for it to exit.
  writer.SetEof();
  cache.Close();
  writer.Close();
}

/* When the source really is exhausted at its stated length, 0 is the correct
 * answer and must keep being served.
 */
TEST(TestFileCache, ExhaustedSourceIsEof)
{
  const CURL pipeUrl("pipe://filecache-eof-test/");

  CPipeFile writer;
  ASSERT_TRUE(writer.OpenForWrite(pipeUrl));

  CFileCache cache{static_cast<unsigned int>(READ_AUDIO_VIDEO)};
  ASSERT_TRUE(cache.Open(pipeUrl));

  constexpr int64_t fileSize = 256 * 1024;
  auto* source = static_cast<CPipeFile*>(cache.GetFileImp());
  ASSERT_NE(nullptr, source);
  source->SetLength(fileSize);

  const std::vector<char> data(fileSize, 'x');
  ASSERT_EQ(static_cast<ssize_t>(fileSize), writer.Write(data.data(), fileSize));
  writer.SetEof();

  std::vector<char> buffer(64 * 1024);
  int64_t total = 0;
  while (total < fileSize)
  {
    const ssize_t read = cache.Read(buffer.data(), buffer.size());
    ASSERT_GT(read, 0) << "read failed at offset " << total << " of a complete file";
    total += read;
  }

  EXPECT_EQ(0, cache.Read(buffer.data(), buffer.size()))
      << "a source exhausted at its stated length did not read as end-of-file";

  cache.Close();
  writer.Close();
}
