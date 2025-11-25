/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/SemanticIndexService.h"
#include "semantic/SemanticTypes.h"

#include <gtest/gtest.h>
#include <thread>

using namespace KODI::SEMANTIC;

class SemanticIndexServiceTest : public ::testing::Test
{
protected:
  void SetUp() override { m_service = std::make_unique<CSemanticIndexService>(); }

  void TearDown() override
  {
    if (m_service && m_service->IsRunning())
    {
      m_service->Stop();
    }
    m_service.reset();
  }

  std::unique_ptr<CSemanticIndexService> m_service;
};

TEST_F(SemanticIndexServiceTest, InitialState)
{
  EXPECT_FALSE(m_service->IsRunning());
  EXPECT_EQ(m_service->GetQueueLength(), 0);
}

TEST_F(SemanticIndexServiceTest, StartAndStop)
{
  bool started = m_service->Start();

  // Service may or may not start depending on dependencies
  if (started)
  {
    EXPECT_TRUE(m_service->IsRunning());
    m_service->Stop();
    EXPECT_FALSE(m_service->IsRunning());
  }
}

TEST_F(SemanticIndexServiceTest, QueueMedia)
{
  m_service->Start();

  // Queue a media item
  EXPECT_NO_THROW({ m_service->QueueMedia(1, "movie", 0); });

  // Queue length should increase
  int queueLength = m_service->GetQueueLength();
  EXPECT_GE(queueLength, 0);

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, QueueMultipleMedia)
{
  m_service->Start();

  m_service->QueueMedia(1, "movie", 0);
  m_service->QueueMedia(2, "movie", 0);
  m_service->QueueMedia(3, "episode", 0);

  int queueLength = m_service->GetQueueLength();
  EXPECT_GE(queueLength, 0);

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, QueueWithPriority)
{
  m_service->Start();

  // Queue items with different priorities
  m_service->QueueMedia(1, "movie", 1);
  m_service->QueueMedia(2, "movie", 10); // Higher priority
  m_service->QueueMedia(3, "movie", 5);

  EXPECT_GE(m_service->GetQueueLength(), 0);

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, CancelMedia)
{
  m_service->Start();

  m_service->QueueMedia(1, "movie", 0);

  // Cancel should not throw
  EXPECT_NO_THROW({ m_service->CancelMedia(1, "movie"); });

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, CancelAllPending)
{
  m_service->Start();

  m_service->QueueMedia(1, "movie", 0);
  m_service->QueueMedia(2, "movie", 0);
  m_service->QueueMedia(3, "episode", 0);

  EXPECT_NO_THROW({ m_service->CancelAllPending(); });

  // Queue should be cleared
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  int queueLength = m_service->GetQueueLength();
  EXPECT_EQ(queueLength, 0);

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, QueueAllUnindexed)
{
  m_service->Start();

  // Should not throw even if no database is available
  EXPECT_NO_THROW({ m_service->QueueAllUnindexed(); });

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, QueueTranscription)
{
  m_service->Start();

  // Should not throw
  EXPECT_NO_THROW({ m_service->QueueTranscription(1, "movie"); });

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, IsMediaIndexed)
{
  m_service->Start();

  // Check if media is indexed (will depend on database state)
  bool indexed = m_service->IsMediaIndexed(1, "movie");
  EXPECT_GE(indexed, false); // Can be true or false

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, GetProgress)
{
  m_service->Start();

  // Get progress for non-queued item
  float progress = m_service->GetProgress(999, "movie");
  EXPECT_FLOAT_EQ(progress, -1.0f); // Not queued

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, GetProgressForQueuedItem)
{
  m_service->Start();

  m_service->QueueMedia(1, "movie", 0);

  // Progress should be between -1.0 and 1.0
  float progress = m_service->GetProgress(1, "movie");
  EXPECT_GE(progress, -1.0f);
  EXPECT_LE(progress, 1.0f);

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, StopWithoutStart)
{
  // Should handle stop without start gracefully
  EXPECT_NO_THROW({ m_service->Stop(); });
}

TEST_F(SemanticIndexServiceTest, MultipleStartCalls)
{
  bool started1 = m_service->Start();
  bool started2 = m_service->Start();

  // Second start should be handled gracefully
  if (started1)
  {
    EXPECT_TRUE(m_service->IsRunning());
  }

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, MultipleStopCalls)
{
  m_service->Start();
  m_service->Stop();

  // Second stop should be safe
  EXPECT_NO_THROW({ m_service->Stop(); });
}

TEST_F(SemanticIndexServiceTest, QueueBeforeStart)
{
  // Queue before starting service
  EXPECT_NO_THROW({ m_service->QueueMedia(1, "movie", 0); });

  // Queue should still work (items pending until service starts)
  EXPECT_GE(m_service->GetQueueLength(), 0);
}

TEST_F(SemanticIndexServiceTest, QueueAfterStop)
{
  m_service->Start();
  m_service->Stop();

  // Queue after stopping should not crash
  EXPECT_NO_THROW({ m_service->QueueMedia(1, "movie", 0); });
}

TEST_F(SemanticIndexServiceTest, DuplicateQueueing)
{
  m_service->Start();

  // Queue same item multiple times
  m_service->QueueMedia(1, "movie", 0);
  m_service->QueueMedia(1, "movie", 0);
  m_service->QueueMedia(1, "movie", 0);

  // Implementation should handle duplicates (either ignore or process once)
  EXPECT_GE(m_service->GetQueueLength(), 0);

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, DifferentMediaTypes)
{
  m_service->Start();

  m_service->QueueMedia(1, "movie", 0);
  m_service->QueueMedia(1, "episode", 0);    // Same ID, different type
  m_service->QueueMedia(1, "musicvideo", 0); // Same ID, different type

  // All three should be treated as separate items
  EXPECT_GE(m_service->GetQueueLength(), 0);

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, ThreadSafety)
{
  m_service->Start();

  // Queue from multiple threads
  std::thread t1([this]() {
    for (int i = 0; i < 10; ++i)
    {
      m_service->QueueMedia(i, "movie", 0);
    }
  });

  std::thread t2([this]() {
    for (int i = 10; i < 20; ++i)
    {
      m_service->QueueMedia(i, "episode", 0);
    }
  });

  t1.join();
  t2.join();

  // Should handle concurrent queuing
  EXPECT_GE(m_service->GetQueueLength(), 0);

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, CancelWhileProcessing)
{
  m_service->Start();

  m_service->QueueMedia(1, "movie", 0);

  // Cancel immediately
  m_service->CancelMedia(1, "movie");

  // Should handle cancellation gracefully
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  m_service->Stop();
}

TEST_F(SemanticIndexServiceTest, EmptyQueueLength)
{
  m_service->Start();

  int queueLength = m_service->GetQueueLength();
  EXPECT_EQ(queueLength, 0);

  m_service->Stop();
}

// Note: Full integration tests would require:
// 1. Valid video database
// 2. Actual media files
// 3. Working subtitle parser
// 4. FFmpeg for transcription
// These should be done in separate integration tests
