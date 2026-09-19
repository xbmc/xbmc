/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/FileCache.h"
#include "filesystem/IFileTypes.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "threads/Event.h"
#include "utils/MemUtils.h"

#if !defined(TARGET_WINDOWS)
#include "platform/posix/ConvUtils.h"
#endif

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
class CGatedFileCacheSource : public IFileCacheSource
{
public:
  enum class Operation
  {
    READ,
    SEEK,
  };

  struct SeekOutcome
  {
    int64_t result;
    int error;
  };

  bool Open(const CURL& url, unsigned int flags) override
  {
    m_firstRead = true;
    m_allowFirstRead.Reset();
    return true;
  }

  void Close() override { m_allowFirstRead.Set(); }

  ssize_t Read(void* buffer, size_t size) override
  {
    if (m_recordNextOperation.exchange(false))
    {
      m_nextOperation.set_value(Operation::READ);
      m_operationRecorded = true;
    }

    if (m_firstRead)
    {
      m_firstRead = false;
      m_firstReadEntered.Set();
      m_allowFirstRead.Wait();
      const size_t count = std::min(size, m_firstReadSize);
      for (size_t index = 0; index < count; ++index)
        static_cast<unsigned char*>(buffer)[index] = static_cast<unsigned char>(index);
      m_position += count;
      return static_cast<ssize_t>(count);
    }

    if (m_positionUncertain)
    {
      static_cast<unsigned char*>(buffer)[0] = 0xa5;
      ++m_position;
      return 1;
    }

    return 0;
  }

  int64_t Seek(int64_t position, int whence) override
  {
    if (m_recordNextOperation.exchange(false))
    {
      m_nextOperation.set_value(Operation::SEEK);
      m_operationRecorded = true;
    }

    m_seekEntered.Set();
    if (m_nextSeekOutcome < m_seekOutcomes.size())
    {
      const SeekOutcome outcome = m_seekOutcomes[m_nextSeekOutcome++];
      m_position = position;
      m_positionUncertain = outcome.result != position;
      if (m_positionUncertain && !m_operationRecorded)
        m_recordNextOperation = true;
      SetLastError(outcome.error);
      return outcome.result;
    }

    m_position = position;
    m_positionUncertain = false;
    SetLastError(0);
    return position;
  }

  int64_t GetLength() override { return 1024 * 1024; }
  int GetChunkSize() override { return 64 * 1024; }
  int IoControl(IOControl request, void* param) override
  {
    if (request == IOControl::SEEK_POSSIBLE)
    {
      SetLastError(EACCES);
      return 1;
    }
    return 0;
  }
  IFile* GetImplementation() override { return nullptr; }

  void AddSeekOutcome(int64_t result, int error)
  {
    m_seekOutcomes.emplace_back(SeekOutcome{result, error});
  }
  void SetFirstReadSize(size_t size) { m_firstReadSize = size; }
  std::future<Operation> GetNextOperation() { return m_nextOperation.get_future(); }
  bool WaitForFirstRead(std::chrono::milliseconds timeout)
  {
    return m_firstReadEntered.Wait(timeout);
  }
  bool WaitForSeek(std::chrono::milliseconds timeout) { return m_seekEntered.Wait(timeout); }
  void AllowFirstRead() { m_allowFirstRead.Set(); }

private:
  bool m_firstRead{true};
  bool m_positionUncertain{false};
  bool m_operationRecorded{false};
  int64_t m_position{0};
  size_t m_firstReadSize{1};
  size_t m_nextSeekOutcome{0};
  std::vector<SeekOutcome> m_seekOutcomes;
  std::atomic<bool> m_recordNextOperation{false};
  std::promise<Operation> m_nextOperation;
  CEvent m_firstReadEntered{true};
  CEvent m_allowFirstRead{true};
  CEvent m_seekEntered{true};
};

//! Serves a large file whose every byte is derived from its position
class CPatternFileCacheSource : public IFileCacheSource
{
public:
  explicit CPatternFileCacheSource(bool seekable) : m_seekable(seekable) {}

  static unsigned char ByteAt(int64_t position)
  {
    return static_cast<unsigned char>(position % 251);
  }

  bool Open(const CURL& url, unsigned int flags) override
  {
    m_position = 0;
    return true;
  }
  void Close() override {}
  ssize_t Read(void* buffer, size_t size) override
  {
    for (size_t index = 0; index < size; ++index)
      static_cast<unsigned char*>(buffer)[index] = ByteAt(m_position + index);
    m_position += size;
    return static_cast<ssize_t>(size);
  }
  int64_t Seek(int64_t position, int whence) override
  {
    if (!m_seekable)
      return -1;
    m_position = position;
    return position;
  }
  int64_t GetLength() override { return int64_t{4} * 1024 * 1024 * 1024; }
  int GetChunkSize() override { return 64 * 1024; }
  int IoControl(IOControl request, void* param) override
  {
    return request == IOControl::SEEK_POSSIBLE && m_seekable ? 1 : 0;
  }
  IFile* GetImplementation() override { return nullptr; }

private:
  const bool m_seekable;
  int64_t m_position{0};
};

class TestFileCache : public CFileCache
{
public:
  TestFileCache(unsigned int flags, std::unique_ptr<IFileCacheSource> source)
    : CFileCache(flags, std::move(source))
  {
  }
};

struct SeekResult
{
  int64_t position;
  DWORD error;
};

SeekResult SeekWithError(CFileCache& cache, int64_t position)
{
  const int64_t result = cache.Seek(position, SEEK_SET);
  return {result, GetLastError()};
}

bool ReadsPattern(CFileCache& cache, size_t count)
{
  std::vector<unsigned char> buffer(64 * 1024);
  while (count > 0)
  {
    const int64_t position = cache.GetPosition();
    const ssize_t read = cache.Read(buffer.data(), std::min(buffer.size(), count));
    if (read <= 0)
      return false;
    for (ssize_t index = 0; index < read; ++index)
    {
      if (buffer[index] != CPatternFileCacheSource::ByteAt(position + index))
        return false;
    }
    count -= read;
  }
  return true;
}

uint64_t ForwardCapacity(CFileCache& cache)
{
  SCacheStatus status{};
  cache.IoControl(IOControl::CACHE_STATUS, &status);
  return status.maxforward;
}
} // namespace

TEST(TestFileCache, NormalSourceSeekReturnsRequestedPosition)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto seekResult =
      std::async(std::launch::async, [&]() { return cache.Seek(256 * 1024, SEEK_SET); });
  sourcePtr->AllowFirstRead();

  const bool seekEntered = sourcePtr->WaitForSeek(5s);
  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  cache.Close();

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(seekEntered);
  ASSERT_TRUE(seekReady);
  EXPECT_EQ(256 * 1024, seekResult.get());
}

TEST(TestFileCache, FailedSourceSeekPropagatesErrorAndQuarantinesReads)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->AddSeekOutcome(-1, ECONNRESET);
  auto nextOperation = sourcePtr->GetNextOperation();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto firstSeek =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();

  const bool firstSeekEntered = sourcePtr->WaitForSeek(5s);
  const bool firstSeekReady = firstSeek.wait_for(5s) == std::future_status::ready;
  SeekResult firstResult{};
  if (firstSeekReady)
    firstResult = firstSeek.get();
  else
  {
    cache.Close();
    ASSERT_TRUE(firstSeekReady);
  }

  auto secondSeek = std::async(std::launch::async, [&]() { return cache.Seek(0, SEEK_SET); });
  const bool operationReady = nextOperation.wait_for(5s) == std::future_status::ready;
  CGatedFileCacheSource::Operation operation{CGatedFileCacheSource::Operation::READ};
  if (operationReady)
    operation = nextOperation.get();
  const bool secondSeekReady = secondSeek.wait_for(5s) == std::future_status::ready;
  cache.Close();

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(firstSeekEntered);
  ASSERT_TRUE(firstSeekReady);
  EXPECT_EQ(-1, firstResult.position);
  EXPECT_EQ(ECONNRESET, firstResult.error);
  ASSERT_TRUE(operationReady);
  EXPECT_EQ(CGatedFileCacheSource::Operation::SEEK, operation);
  ASSERT_TRUE(secondSeekReady);
  EXPECT_EQ(0, secondSeek.get());
}

TEST(TestFileCache, WrongPositiveSourceSeekResultIsFailure)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->AddSeekOutcome(256 * 1024 + 1, 0);
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto seekResult =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();

  const bool seekEntered = sourcePtr->WaitForSeek(5s);
  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  SeekResult result{};
  if (seekReady)
    result = seekResult.get();
  cache.Close();

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(seekEntered);
  ASSERT_TRUE(seekReady);
  EXPECT_EQ(-1, result.position);
  EXPECT_EQ(EIO, result.error);
}

TEST(TestFileCache, FailedSourceSeekLeavesCacheReadPositionUnchanged)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->SetFirstReadSize(64);
  sourcePtr->AddSeekOutcome(-1, ECONNRESET);
  sourcePtr->AddSeekOutcome(-1, ECONNRESET);
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto firstSeek =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();
  const bool firstSeekReady = firstSeek.wait_for(5s) == std::future_status::ready;
  SeekResult firstResult{};
  if (firstSeekReady)
    firstResult = firstSeek.get();
  else
  {
    cache.Close();
    ASSERT_TRUE(firstSeekReady);
  }

  auto secondSeek = std::async(std::launch::async, [&]() { return SeekWithError(cache, 32); });
  const bool secondSeekReady = secondSeek.wait_for(5s) == std::future_status::ready;
  SeekResult secondResult{};
  if (secondSeekReady)
    secondResult = secondSeek.get();
  else
  {
    cache.Close();
    ASSERT_TRUE(secondSeekReady);
  }

  unsigned char value = 0xff;
  const ssize_t bytesRead = cache.Read(&value, 1);
  const int64_t position = cache.GetPosition();
  cache.Close();

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(firstSeekReady);
  ASSERT_TRUE(secondSeekReady);
  EXPECT_EQ(-1, firstResult.position);
  EXPECT_EQ(-1, secondResult.position);
  EXPECT_EQ(1, bytesRead);
  EXPECT_EQ(0, value);
  EXPECT_EQ(1, position);
}

TEST(TestFileCache, ReadFailsPromptlyAfterQuarantinedCacheDrains)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->AddSeekOutcome(-1, ECONNRESET);
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto seekResult =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();
  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  if (!seekReady)
  {
    cache.Close();
    ASSERT_TRUE(seekReady);
  }

  unsigned char value = 0xff;
  const ssize_t cachedRead = cache.Read(&value, 1);
  auto failedRead = std::async(std::launch::async,
                               [&]()
                               {
                                 unsigned char nextValue{};
                                 const ssize_t result = cache.Read(&nextValue, 1);
                                 return std::pair{result, GetLastError()};
                               });
  const bool readFailedPromptly = failedRead.wait_for(1s) == std::future_status::ready;
  cache.Close();
  const bool failedReadReady = failedRead.wait_for(5s) == std::future_status::ready;

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(seekReady);
  EXPECT_EQ(-1, seekResult.get().position);
  EXPECT_EQ(1, cachedRead);
  EXPECT_EQ(0, value);
  ASSERT_TRUE(readFailedPromptly);
  ASSERT_TRUE(failedReadReady);
  const auto [readResult, readError] = failedRead.get();
  EXPECT_EQ(-1, readResult);
  EXPECT_EQ(ECONNRESET, readError);
}

TEST(TestFileCache, DefaultSizedCacheGrowsToTheContentRate)
{
  // A minute at this rate is 90 MiB forward, past the 48 MiB a default cache holds
  uint32_t rate = 1536 * 1024;
  KODI::MEMORY::MemoryStatus memory{};
  KODI::MEMORY::GetMemoryStatus(&memory);
  if (memory.totalPhys / 16 < 128 * 1024 * 1024)
    GTEST_SKIP() << "not enough installed memory for the cache to grow";

  TestFileCache cache{READ_AUDIO_VIDEO, std::make_unique<CPatternFileCacheSource>(true)};
  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readsBefore = ReadsPattern(cache, 1024 * 1024);
  const uint64_t capacityBefore = ForwardCapacity(cache);

  cache.IoControl(IOControl::CACHE_SETRATE, &rate);
  const uint64_t capacityAfter = ForwardCapacity(cache);
  const bool readsAfter = ReadsPattern(cache, 1024 * 1024);
  cache.Close();

  EXPECT_TRUE(readsBefore);
  EXPECT_GT(capacityAfter, capacityBefore);
  EXPECT_TRUE(readsAfter);
}

TEST(TestFileCache, UserChosenCacheSizeIsKept)
{
  uint32_t rate = 1536 * 1024;
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  ASSERT_TRUE(settings->SetInt(CSettings::SETTING_FILECACHE_MEMORYSIZE, 96));

  TestFileCache cache{READ_AUDIO_VIDEO, std::make_unique<CPatternFileCacheSource>(true)};
  const bool opened = cache.Open(CURL{"mock://server/movie.mkv"});
  const uint64_t capacityBefore = ForwardCapacity(cache);
  cache.IoControl(IOControl::CACHE_SETRATE, &rate);
  const uint64_t capacityAfter = ForwardCapacity(cache);
  cache.Close();
  settings->GetSetting(CSettings::SETTING_FILECACHE_MEMORYSIZE)->Reset();

  ASSERT_TRUE(opened);
  EXPECT_EQ(capacityBefore, capacityAfter);
}

TEST(TestFileCache, UnseekableSourceKeepsItsCache)
{
  uint32_t rate = 1536 * 1024;

  TestFileCache cache{READ_AUDIO_VIDEO, std::make_unique<CPatternFileCacheSource>(false)};
  ASSERT_TRUE(cache.Open(CURL{"mock://server/stream.ts"}));
  const bool readsBefore = ReadsPattern(cache, 1024 * 1024);
  const uint64_t capacityBefore = ForwardCapacity(cache);

  cache.IoControl(IOControl::CACHE_SETRATE, &rate);
  const uint64_t capacityAfter = ForwardCapacity(cache);
  const bool readsAfter = ReadsPattern(cache, 1024 * 1024);
  cache.Close();

  EXPECT_TRUE(readsBefore);
  EXPECT_EQ(capacityBefore, capacityAfter);
  EXPECT_TRUE(readsAfter);
}
