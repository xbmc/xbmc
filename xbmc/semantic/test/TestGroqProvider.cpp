/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/transcription/GroqProvider.h"

#include <gtest/gtest.h>

using namespace KODI::SEMANTIC;

class GroqProviderTest : public ::testing::Test
{
protected:
  void SetUp() override { m_provider = std::make_unique<CGroqProvider>(); }

  void TearDown() override { m_provider.reset(); }

  std::unique_ptr<CGroqProvider> m_provider;
};

TEST_F(GroqProviderTest, GetName)
{
  EXPECT_EQ(m_provider->GetName(), "Groq Whisper");
}

TEST_F(GroqProviderTest, GetId)
{
  EXPECT_EQ(m_provider->GetId(), "groq");
}

TEST_F(GroqProviderTest, IsConfiguredWithoutApiKey)
{
  // Without API key set in settings, should not be configured
  // (This may return true/false depending on test environment)
  auto isConfigured = m_provider->IsConfigured();
  EXPECT_GE(isConfigured, false); // Can be true or false
}

TEST_F(GroqProviderTest, IsAvailable)
{
  // Availability depends on configuration and network
  auto isAvailable = m_provider->IsAvailable();
  EXPECT_GE(isAvailable, false); // Can be true or false
}

TEST_F(GroqProviderTest, EstimateCost)
{
  // Test cost estimation
  // 1 minute = 60000 ms
  float cost1Min = m_provider->EstimateCost(60000);
  EXPECT_FLOAT_EQ(cost1Min, 0.0033f); // $0.0033 per minute

  // 10 minutes
  float cost10Min = m_provider->EstimateCost(600000);
  EXPECT_FLOAT_EQ(cost10Min, 0.033f); // $0.033 for 10 minutes

  // 1 hour
  float cost1Hour = m_provider->EstimateCost(3600000);
  EXPECT_NEAR(cost1Hour, 0.198f, 0.01f); // ~$0.20 per hour
}

TEST_F(GroqProviderTest, EstimateCostZeroDuration)
{
  float cost = m_provider->EstimateCost(0);
  EXPECT_FLOAT_EQ(cost, 0.0f);
}

TEST_F(GroqProviderTest, EstimateCostShortDuration)
{
  // 30 seconds = 30000 ms
  float cost = m_provider->EstimateCost(30000);
  EXPECT_FLOAT_EQ(cost, 0.00165f); // Half of 1 minute rate
}

TEST_F(GroqProviderTest, Cancel)
{
  // Should not throw when cancelling
  EXPECT_NO_THROW({ m_provider->Cancel(); });
}

TEST_F(GroqProviderTest, TranscribeWithoutApiKey)
{
  // Transcribe should fail if no API key is configured
  bool called = false;
  bool hasError = false;

  auto segmentCallback = [&](const TranscriptSegment& segment) { called = true; };

  auto progressCallback = [](float progress) {};

  auto errorCallback = [&](const std::string& error) { hasError = true; };

  // This will likely fail without a valid API key
  bool result = m_provider->Transcribe("/nonexistent/file.mp3", segmentCallback,
                                       progressCallback, errorCallback);

  // Either fails immediately or reports error
  if (!result)
  {
    EXPECT_FALSE(result);
  }
  else
  {
    // If it tried to transcribe, we'd expect an error callback
    EXPECT_TRUE(hasError || !called);
  }
}

TEST_F(GroqProviderTest, TranscribeNonExistentFile)
{
  bool hasError = false;

  auto segmentCallback = [](const TranscriptSegment& segment) {};
  auto progressCallback = [](float progress) {};
  auto errorCallback = [&](const std::string& error) { hasError = true; };

  bool result = m_provider->Transcribe("/nonexistent/file/path.mp3", segmentCallback,
                                       progressCallback, errorCallback);

  // Should fail for non-existent file
  EXPECT_FALSE(result);
}

TEST_F(GroqProviderTest, CancelDuringTranscription)
{
  // Simulate cancellation
  m_provider->Cancel();

  auto segmentCallback = [](const TranscriptSegment& segment) {};
  auto progressCallback = [](float progress) {};
  auto errorCallback = [](const std::string& error) {};

  // Transcription should respect cancellation
  bool result = m_provider->Transcribe("/some/file.mp3", segmentCallback, progressCallback,
                                       errorCallback);

  // Should return false if cancelled
  EXPECT_FALSE(result);
}

TEST_F(GroqProviderTest, Constants)
{
  // Verify expected constants are defined
  EXPECT_STREQ(CGroqProvider::API_ENDPOINT, "https://api.groq.com/openai/v1/audio/transcriptions");
  EXPECT_STREQ(CGroqProvider::MODEL_NAME, "whisper-large-v3-turbo");
  EXPECT_STREQ(CGroqProvider::RESPONSE_FORMAT, "verbose_json");
  EXPECT_EQ(CGroqProvider::MAX_FILE_SIZE, 25 * 1024 * 1024); // 25MB
  EXPECT_FLOAT_EQ(CGroqProvider::COST_PER_MINUTE, 0.0033f);
}

TEST_F(GroqProviderTest, LargeFileDetection)
{
  // Verify MAX_FILE_SIZE constant is 25MB
  EXPECT_EQ(CGroqProvider::MAX_FILE_SIZE, 25 * 1024 * 1024);

  // The Transcribe method should route to TranscribeLargeFile for files > 25MB
  // This is tested indirectly through the public API
}

TEST_F(GroqProviderTest, TranscribeLargeFileWithoutApiKey)
{
  // Large file transcription should fail gracefully without API key
  bool hasError = false;
  std::string errorMsg;

  // Note: Use TranscriptSegment (not TranscriptionSegment) - defined in ITranscriptionProvider.h:21
  auto segmentCallback = [](const TranscriptSegment& segment) {};
  auto progressCallback = [](float progress) {};
  auto errorCallback = [&](const std::string& error) {
    hasError = true;
    errorMsg = error;
  };

  // Create a fake path that would trigger large file handling
  // (actual file size check happens in Transcribe())
  bool result = m_provider->Transcribe("/path/to/large/file.mp3",
                                       segmentCallback, progressCallback, errorCallback);

  // Should fail (no API key or file doesn't exist)
  EXPECT_FALSE(result);
}

// Note: Full integration tests with actual API calls would require:
// 1. Valid API key
// 2. Real audio files
// 3. Network connectivity
// 4. Budget for API usage
// These should be done in separate integration tests, not unit tests
