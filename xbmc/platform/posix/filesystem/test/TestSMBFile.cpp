/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PasswordManager.h"
#include "threads/Event.h"

#include "platform/posix/filesystem/SMBFile.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <future>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
class CFakeSMBFileOperations : public SMBFileRecovery::ISMBFileOperations
{
public:
  void PauseNextResolve()
  {
    m_pauseNextResolve = true;
    m_resolveEntered.Reset();
    m_allowResolve.Reset();
  }
  bool WaitForResolveEntry(std::chrono::milliseconds timeout)
  {
    return m_resolveEntered.Wait(timeout);
  }
  void ResumeResolve() { m_allowResolve.Set(); }
  void FailNextOpen(int error) { m_nextOpenError = error; }
  void FailNextRead(int error) { m_nextReadError = error; }
  void FailAllReads(int error) { m_readError = error; }
  void ReturnUnexpectedEofOnNextRead() { m_unexpectedEofOnNextRead = true; }
  void FailNextSeek(int error) { m_nextSeekError = error; }
  void FailNextAbsoluteSeek(int error) { m_nextAbsoluteSeekError = error; }
  void ReturnNextSeekPosition(int64_t position) { m_nextSeekPosition = position; }
  void SetFileSize(int64_t size) { m_fileSize = size; }
  void SetCurrentHandlePosition(int64_t position)
  {
    ASSERT_EQ(1u, m_handles.size());
    m_handles.begin()->second.position = position;
  }
  int OpenCount() const { return m_openCount; }
  const std::vector<std::string>& OpenPaths() const { return m_openPaths; }

  void Init() override {}
  void AddActiveConnection() override {}
  void AddIdleConnection() override {}
  void SetActivityTime() override {}
  bool IsValid() const override { return true; }
  CCriticalSection& GetCriticalSection() override { return m_criticalSection; }
  CURL Resolve(const CURL& url) override
  {
    if (m_pauseNextResolve.exchange(false))
    {
      m_resolveEntered.Set();
      m_allowResolve.Wait();
    }
    return url;
  }
  std::string URLEncode(const CURL& url) override { return url.Get(); }

  int Open(const std::string& path, int flags) override
  {
    ++m_openCount;
    m_openPaths.emplace_back(path);
    if (m_nextOpenError != 0)
    {
      errno = m_nextOpenError;
      m_nextOpenError = 0;
      return -1;
    }

    const int fd = m_nextFd++;
    m_handles.emplace(fd, Handle{});
    return fd;
  }

  int Create(const std::string& path) override { return Open(path, 0); }

  int Stat(const std::string& path, struct stat* buffer) override
  {
    buffer->st_size = m_fileSize;
    return 0;
  }

  int FStat(int fd, struct stat* buffer) override
  {
    if (!m_handles.contains(fd))
    {
      errno = EBADF;
      return -1;
    }

    buffer->st_size = m_fileSize;
    return 0;
  }

  ssize_t Read(int fd, void* buffer, size_t size) override
  {
    auto it = m_handles.find(fd);
    if (it == m_handles.end())
    {
      errno = EBADF;
      return -1;
    }

    if (m_nextReadError != 0)
    {
      errno = m_nextReadError;
      m_nextReadError = 0;
      return -1;
    }

    if (m_readError != 0)
    {
      errno = m_readError;
      return -1;
    }

    if (m_unexpectedEofOnNextRead)
    {
      m_unexpectedEofOnNextRead = false;
      it->second.unexpectedEof = true;
    }

    if (it->second.unexpectedEof)
      return 0;

    const size_t count = static_cast<size_t>(
        std::min<int64_t>(size, std::max<int64_t>(0, m_fileSize - it->second.position)));
    std::memset(buffer, 0x5a, count);
    it->second.position += count;
    return static_cast<ssize_t>(count);
  }

  int64_t Seek(int fd, int64_t offset, int whence) override
  {
    auto it = m_handles.find(fd);
    if (it == m_handles.end())
    {
      errno = EBADF;
      return -1;
    }

    if (m_nextSeekError != 0)
    {
      errno = m_nextSeekError;
      m_nextSeekError = 0;
      return -1;
    }

    if (whence == SEEK_SET && m_nextAbsoluteSeekError != 0)
    {
      errno = m_nextAbsoluteSeekError;
      m_nextAbsoluteSeekError = 0;
      return -1;
    }

    if (m_nextSeekPosition >= 0)
    {
      const int64_t result = m_nextSeekPosition;
      m_nextSeekPosition = -1;
      it->second.position = result;
      return result;
    }

    int64_t target = offset;
    if (whence == SEEK_CUR)
      target += it->second.position;
    else if (whence == SEEK_END)
      target += m_fileSize;
    else if (whence != SEEK_SET)
    {
      errno = EINVAL;
      return -1;
    }

    if (target < 0)
    {
      errno = EINVAL;
      return -1;
    }

    it->second.position = target;
    return target;
  }

  int Close(int fd) override { return m_handles.erase(fd) == 1 ? 0 : -1; }

  ssize_t Write(int fd, const void* buffer, size_t size) override
  {
    auto it = m_handles.find(fd);
    if (it == m_handles.end())
    {
      errno = EBADF;
      return -1;
    }

    it->second.position += size;
    m_fileSize = std::max<int64_t>(m_fileSize, it->second.position);
    return static_cast<ssize_t>(size);
  }

  int Unlink(const std::string& path) override { return 0; }
  int Rename(const std::string& from, const std::string& to) override { return 0; }

private:
  struct Handle
  {
    int64_t position{0};
    bool unexpectedEof{false};
  };

  int m_nextFd{100};
  int m_openCount{0};
  int m_nextOpenError{0};
  int m_nextReadError{0};
  int m_readError{0};
  int m_nextSeekError{0};
  int m_nextAbsoluteSeekError{0};
  int64_t m_nextSeekPosition{-1};
  int64_t m_fileSize{4096};
  bool m_unexpectedEofOnNextRead{false};
  std::atomic<bool> m_pauseNextResolve{false};
  CEvent m_resolveEntered{true};
  CEvent m_allowResolve{true};
  CCriticalSection m_criticalSection;
  std::unordered_map<int, Handle> m_handles;
  std::vector<std::string> m_openPaths;
};

class TestSMBFile : public CSMBFile
{
public:
  explicit TestSMBFile(SMBFileRecovery::ISMBFileOperations& operations) : CSMBFile(operations) {}
};

class TestSMBFileRecovery : public testing::Test
{
protected:
  void SetUp() override { CPasswordManager::GetInstance().Clear(); }
  void TearDown() override { CPasswordManager::GetInstance().Clear(); }
};
} // namespace

TEST_F(TestSMBFileRecovery, NormalSeekReturnsExactLowLevelPosition)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  EXPECT_EQ(2048, file.Seek(2048, SEEK_SET));
  EXPECT_EQ(2048, file.GetPosition());
}

TEST_F(TestSMBFileRecovery, SeekEndUsesCurrentFileSize)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};

  operations.SetFileSize(4096);
  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  operations.SetFileSize(8192);

  EXPECT_EQ(8192, file.Seek(0, SEEK_END));
  EXPECT_EQ(8192, file.GetPosition());
  EXPECT_EQ(8192, file.GetLength());
}

TEST_F(TestSMBFileRecovery, RecoveredSeekEndUsesCurrentFileSize)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  operations.SetFileSize(4096);
  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));
  operations.SetFileSize(2048);

  EXPECT_EQ(2048, file.Seek(0, SEEK_END));
  EXPECT_EQ(2048, file.GetPosition());
  EXPECT_EQ(2048, file.GetLength());
  EXPECT_EQ(2, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, RecoveredSeekEndAppliesNegativeOffsetToCurrentFileSize)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  operations.SetFileSize(4096);
  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));
  operations.SetFileSize(8192);

  EXPECT_EQ(8092, file.Seek(-100, SEEK_END));
  EXPECT_EQ(8092, file.GetPosition());
  EXPECT_EQ(8192, file.GetLength());
  EXPECT_EQ(2, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, FailedSeekEndValidationInvalidatesDescriptor)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  operations.SetFileSize(8192);
  operations.FailNextAbsoluteSeek(EACCES);

  EXPECT_EQ(-1, file.Seek(0, SEEK_END));
  EXPECT_EQ(EACCES, errno);
  EXPECT_EQ(-1, file.GetPosition());
}

TEST_F(TestSMBFileRecovery, WritableRelativeSeekKeepsPhysicalCursorSemantics)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  std::array<std::byte, 100> buffer{};

  ASSERT_TRUE(file.OpenForWrite(CURL{"smb://127.0.0.1/share/output.mkv"}));
  ASSERT_EQ(static_cast<ssize_t>(buffer.size()), file.Write(buffer.data(), buffer.size()));

  EXPECT_EQ(50, file.Seek(-50, SEEK_CUR));
  EXPECT_EQ(50, file.GetPosition());
}

TEST_F(TestSMBFileRecovery, ResetBeforeSeekReopensAtRequestedPosition)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));

  EXPECT_EQ(3072, file.Seek(3072, SEEK_SET));
  EXPECT_EQ(3072, file.GetPosition());
  EXPECT_EQ(2, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, InterruptedSequentialReadRecoversAtLogicalPosition)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  operations.FailNextRead(ECONNRESET);

  EXPECT_EQ(static_cast<ssize_t>(buffer.size()), file.Read(buffer.data(), buffer.size()));
  EXPECT_EQ(static_cast<int64_t>(buffer.size()), file.GetPosition());
  EXPECT_EQ(2, operations.OpenCount());
  EXPECT_EQ(0, errno);
}

TEST_F(TestSMBFileRecovery, ResetDuringSeekReopensAtRequestedPosition)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  operations.FailNextSeek(ECONNRESET);

  EXPECT_EQ(3072, file.Seek(3072, SEEK_SET));
  EXPECT_EQ(3072, file.GetPosition());
  EXPECT_EQ(2, operations.OpenCount());
  EXPECT_EQ(0, errno);
}

TEST_F(TestSMBFileRecovery, RelativeSeekUsesAuthoritativeLogicalPosition)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(static_cast<ssize_t>(buffer.size()), file.Read(buffer.data(), buffer.size()));
  operations.SetCurrentHandlePosition(1000);

  EXPECT_EQ(20, file.Seek(4, SEEK_CUR));
  EXPECT_EQ(20, file.GetPosition());
}

TEST_F(TestSMBFileRecovery, WrongPositiveSeekResultIsFailure)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  operations.ReturnNextSeekPosition(1024);

  EXPECT_EQ(-1, file.Seek(2048, SEEK_SET));
  EXPECT_EQ(EIO, errno);
  EXPECT_EQ(-1, file.GetPosition());
}

TEST_F(TestSMBFileRecovery, SeekRetriesReconnectableReopenFailure)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  operations.FailNextSeek(ECONNRESET);
  operations.FailNextOpen(ECONNRESET);

  EXPECT_EQ(3072, file.Seek(3072, SEEK_SET));
  EXPECT_EQ(3072, file.GetPosition());
  EXPECT_EQ(3, operations.OpenCount());
  EXPECT_EQ(0, errno);
}

TEST_F(TestSMBFileRecovery, NewerSeekSupersedesOverlappingReadRecovery)
{
  using namespace std::chrono_literals;

  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  std::array<std::byte, 16> recoveryBuffer{};
  std::array<std::byte, 8> postSeekBuffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  operations.PauseNextResolve();
  operations.FailNextRead(ECONNRESET);
  auto readResult = std::async(std::launch::async,
                               [&]()
                               {
                                 const ssize_t result =
                                     file.Read(recoveryBuffer.data(), recoveryBuffer.size());
                                 return std::pair{result, errno};
                               });

  const bool resolveEntered = operations.WaitForResolveEntry(5s);
  auto seekResult = std::async(std::launch::async, [&]() { return file.Seek(3072, SEEK_SET); });
  const bool seekCompletedBeforeRelease = seekResult.wait_for(5s) == std::future_status::ready;
  operations.ResumeResolve();

  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  ASSERT_EQ(std::future_status::ready, readResult.wait_for(5s));
  ASSERT_TRUE(resolveEntered);
  ASSERT_TRUE(seekCompletedBeforeRelease);
  ASSERT_TRUE(seekReady);
  EXPECT_EQ(3072, seekResult.get());
  const auto [bytesRead, readError] = readResult.get();
  EXPECT_EQ(-1, bytesRead);
  EXPECT_EQ(ECANCELED, readError);
  EXPECT_EQ(3072, file.GetPosition());
  EXPECT_EQ(static_cast<ssize_t>(postSeekBuffer.size()),
            file.Read(postSeekBuffer.data(), postSeekBuffer.size()));
  EXPECT_EQ(3080, file.GetPosition());
}

TEST_F(TestSMBFileRecovery, SupersededSeeksDoNotConsumeReconnectAttempts)
{
  using namespace std::chrono_literals;

  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 8> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));

  auto seekWithError = [&](int64_t target)
  {
    const int64_t result = file.Seek(target, SEEK_SET);
    return std::pair{result, errno};
  };

  operations.PauseNextResolve();
  auto firstSeek = std::async(std::launch::async, seekWithError, 1024);
  const bool firstEntered = operations.WaitForResolveEntry(5s);
  operations.PauseNextResolve();
  auto secondSeek = std::async(std::launch::async, seekWithError, 1536);
  const bool secondEntered = operations.WaitForResolveEntry(5s);
  operations.PauseNextResolve();
  auto thirdSeek = std::async(std::launch::async, seekWithError, 2048);
  const bool thirdEntered = operations.WaitForResolveEntry(5s);

  auto finalSeek = std::async(std::launch::async, seekWithError, 3072);
  const bool finalReady = finalSeek.wait_for(5s) == std::future_status::ready;
  operations.ResumeResolve();

  const bool firstReady = firstSeek.wait_for(5s) == std::future_status::ready;
  const bool secondReady = secondSeek.wait_for(5s) == std::future_status::ready;
  const bool thirdReady = thirdSeek.wait_for(5s) == std::future_status::ready;

  ASSERT_TRUE(firstEntered);
  ASSERT_TRUE(secondEntered);
  ASSERT_TRUE(thirdEntered);
  ASSERT_TRUE(finalReady);
  ASSERT_TRUE(firstReady);
  ASSERT_TRUE(secondReady);
  ASSERT_TRUE(thirdReady);
  const auto firstResult = firstSeek.get();
  const auto secondResult = secondSeek.get();
  const auto thirdResult = thirdSeek.get();
  const auto finalResult = finalSeek.get();
  EXPECT_EQ(-1, firstResult.first);
  EXPECT_EQ(ECANCELED, firstResult.second);
  EXPECT_EQ(-1, secondResult.first);
  EXPECT_EQ(ECANCELED, secondResult.second);
  EXPECT_EQ(-1, thirdResult.first);
  EXPECT_EQ(ECANCELED, thirdResult.second);
  EXPECT_EQ(3072, finalResult.first);
  EXPECT_EQ(0, finalResult.second);
  EXPECT_EQ(2, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, ReadCannotOvertakePendingSeekReopen)
{
  using namespace std::chrono_literals;

  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 8> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));

  operations.PauseNextResolve();
  auto seekResult = std::async(std::launch::async, [&]() { return file.Seek(3072, SEEK_SET); });
  EXPECT_TRUE(operations.WaitForResolveEntry(5s));
  auto readResult =
      std::async(std::launch::async, [&]() { return file.Read(buffer.data(), buffer.size()); });

  const auto readStatus = readResult.wait_for(5s);
  operations.ResumeResolve();
  ASSERT_EQ(std::future_status::ready, readStatus);
  EXPECT_EQ(-1, readResult.get());
  ASSERT_EQ(std::future_status::ready, seekResult.wait_for(5s));
  EXPECT_EQ(3072, seekResult.get());
  EXPECT_EQ(3072, file.GetPosition());
}

TEST_F(TestSMBFileRecovery, ReopenClearsDeadHandleUnexpectedEof)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  operations.ReturnUnexpectedEofOnNextRead();

  EXPECT_EQ(static_cast<ssize_t>(buffer.size()), file.Read(buffer.data(), buffer.size()));
  EXPECT_EQ(static_cast<int64_t>(buffer.size()), file.GetPosition());
  EXPECT_EQ(2, operations.OpenCount());
  EXPECT_EQ(0, errno);
}

TEST_F(TestSMBFileRecovery, FailedRecoveredSeekReturnsFailure)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));
  operations.FailNextSeek(EACCES);

  EXPECT_EQ(-1, file.Seek(2048, SEEK_SET));
  EXPECT_EQ(-1, file.GetPosition());
  EXPECT_EQ(2, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, FailedReconnectReturnsPromptly)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));
  operations.FailNextOpen(EACCES);

  EXPECT_EQ(-1, file.Seek(2048, SEEK_SET));
  EXPECT_EQ(EACCES, errno);
  EXPECT_EQ(2, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, PendingDelegatedRecoveryKeepsCachedLength)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));

  EXPECT_EQ(4096, file.GetLength());
  EXPECT_EQ(static_cast<ssize_t>(buffer.size()), file.Read(buffer.data(), buffer.size()));
  EXPECT_EQ(2, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, DelegatedRecoveryStopsAfterNonReconnectableReopenFailure)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));
  operations.FailNextOpen(EACCES);

  EXPECT_EQ(-1, file.Read(buffer.data(), buffer.size()));
  EXPECT_EQ(EACCES, errno);
  EXPECT_EQ(-1, file.Read(buffer.data(), buffer.size()));
  EXPECT_EQ(2, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, DelegatedReadRecoveryIsBoundedWithoutProgress)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));
  operations.FailAllReads(ECONNRESET);

  for (unsigned int attempt = 0; attempt < 10; ++attempt)
    EXPECT_EQ(-1, file.Read(buffer.data(), buffer.size()));

  EXPECT_EQ(4, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, RecoveredSeeksDoNotRefreshRetryBudgetWithoutReadProgress)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  bool retry = false;
  std::array<std::byte, 16> buffer{};

  ASSERT_TRUE(file.Open(CURL{"smb://127.0.0.1/share/movie.mkv"}));
  ASSERT_EQ(0, file.IoControl(IOControl::SET_RETRY, &retry));

  for (int64_t target : {1024, 2048, 3072})
  {
    operations.FailNextRead(ECONNRESET);
    ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));
    EXPECT_EQ(target, file.Seek(target, SEEK_SET));
  }

  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(-1, file.Read(buffer.data(), buffer.size()));
  EXPECT_EQ(-1, file.Seek(0, SEEK_SET));
  EXPECT_EQ(4, operations.OpenCount());
}

TEST_F(TestSMBFileRecovery, ReconnectUsesRememberedCredentials)
{
  CFakeSMBFileOperations operations;
  TestSMBFile file{operations};
  std::array<std::byte, 16> buffer{};
  CURL url{"smb://127.0.0.1/share/movie.mkv"};
  url.SetUserName("kodi");
  url.SetPassword("secret");

  ASSERT_TRUE(file.Open(url));
  operations.FailNextRead(ECONNRESET);
  ASSERT_EQ(static_cast<ssize_t>(buffer.size()), file.Read(buffer.data(), buffer.size()));

  ASSERT_EQ(2u, operations.OpenPaths().size());
  for (const std::string& path : operations.OpenPaths())
    EXPECT_EQ("smb://kodi:secret@127.0.0.1/share/movie.mkv", path);
}

TEST_F(TestSMBFileRecovery, ReconnectableReadErrors)
{
#ifdef ENETRESET
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ENETRESET));
#endif
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ECONNRESET));
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ECONNABORTED));
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ENOTCONN));
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(EPIPE));
  EXPECT_TRUE(SMBFileRecovery::IsReconnectableReadError(ETIMEDOUT));

  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(0));
  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(EACCES));
  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(ENOENT));
  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(EINVAL));
  EXPECT_FALSE(SMBFileRecovery::IsReconnectableReadError(EIO));
}

TEST_F(TestSMBFileRecovery, ZeroBytesBeforeKnownFileEndIsInvalidEof)
{
  EXPECT_FALSE(SMBFileRecovery::IsValidEof(4095, 4096));
}

TEST_F(TestSMBFileRecovery, ZeroBytesAtOrBeyondKnownFileEndIsValidEof)
{
  EXPECT_TRUE(SMBFileRecovery::IsValidEof(4096, 4096));
  EXPECT_TRUE(SMBFileRecovery::IsValidEof(8192, 4096));
}
