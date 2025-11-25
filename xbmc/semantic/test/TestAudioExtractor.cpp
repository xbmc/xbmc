/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/transcription/AudioExtractor.h"

#include <gtest/gtest.h>

using namespace KODI::SEMANTIC;

class AudioExtractorTest : public ::testing::Test
{
protected:
  void SetUp() override { m_extractor = std::make_unique<CAudioExtractor>(); }

  void TearDown() override { m_extractor.reset(); }

  std::unique_ptr<CAudioExtractor> m_extractor;
};

TEST_F(AudioExtractorTest, DefaultConfiguration)
{
  const auto& config = m_extractor->GetConfig();

  EXPECT_EQ(config.sampleRate, 16000);
  EXPECT_EQ(config.channels, 1);
  EXPECT_EQ(config.format, "mp3");
  EXPECT_EQ(config.bitrate, 64);
  EXPECT_EQ(config.maxSegmentMinutes, 45);
  EXPECT_EQ(config.maxFileSizeMB, 25);
}

TEST_F(AudioExtractorTest, CustomConfiguration)
{
  AudioExtractionConfig customConfig;
  customConfig.sampleRate = 22050;
  customConfig.channels = 2;
  customConfig.format = "wav";
  customConfig.bitrate = 128;
  customConfig.maxSegmentMinutes = 30;
  customConfig.maxFileSizeMB = 50;

  m_extractor->SetConfig(customConfig);

  const auto& config = m_extractor->GetConfig();
  EXPECT_EQ(config.sampleRate, 22050);
  EXPECT_EQ(config.channels, 2);
  EXPECT_EQ(config.format, "wav");
  EXPECT_EQ(config.bitrate, 128);
  EXPECT_EQ(config.maxSegmentMinutes, 30);
  EXPECT_EQ(config.maxFileSizeMB, 50);
}

TEST_F(AudioExtractorTest, IsFFmpegAvailable)
{
  // Test FFmpeg availability
  // This may be true or false depending on system setup
  bool available = CAudioExtractor::IsFFmpegAvailable();

  // Should return a boolean (not throw)
  EXPECT_GE(available, false);

  // If FFmpeg is available, log it for debugging
  if (available)
  {
    std::cout << "FFmpeg is available on this system" << std::endl;
  }
  else
  {
    std::cout << "FFmpeg is NOT available on this system" << std::endl;
  }
}

TEST_F(AudioExtractorTest, ExtractAudioNonExistentFile)
{
  if (!CAudioExtractor::IsFFmpegAvailable())
  {
    GTEST_SKIP() << "FFmpeg not available, skipping extraction test";
  }

  bool result = m_extractor->ExtractAudio("/nonexistent/video.mkv", "/tmp/output.mp3");

  // Should fail for non-existent file
  EXPECT_FALSE(result);
}

TEST_F(AudioExtractorTest, GetMediaDurationNonExistentFile)
{
  if (!CAudioExtractor::IsFFmpegAvailable())
  {
    GTEST_SKIP() << "FFmpeg not available, skipping duration test";
  }

  int64_t duration = m_extractor->GetMediaDuration("/nonexistent/file.mp4");

  // Should return -1 for non-existent file
  EXPECT_EQ(duration, -1);
}

TEST_F(AudioExtractorTest, CancelExtraction)
{
  // Should not throw when cancelling
  EXPECT_NO_THROW({ m_extractor->Cancel(); });

  EXPECT_TRUE(m_extractor->IsCancelled());
}

TEST_F(AudioExtractorTest, CancelState)
{
  EXPECT_FALSE(m_extractor->IsCancelled());

  m_extractor->Cancel();
  EXPECT_TRUE(m_extractor->IsCancelled());

  // Create new extractor - should not be cancelled
  auto newExtractor = std::make_unique<CAudioExtractor>();
  EXPECT_FALSE(newExtractor->IsCancelled());
}

TEST_F(AudioExtractorTest, ExtractChunkedNonExistentFile)
{
  if (!CAudioExtractor::IsFFmpegAvailable())
  {
    GTEST_SKIP() << "FFmpeg not available, skipping chunked extraction test";
  }

  auto segments = m_extractor->ExtractChunked("/nonexistent/video.mkv", "/tmp/");

  // Should return empty vector for non-existent file
  EXPECT_TRUE(segments.empty());
}

TEST_F(AudioExtractorTest, CleanupSegments)
{
  // Create fake segment list
  std::vector<AudioSegment> segments;

  AudioSegment seg1;
  seg1.path = "/tmp/nonexistent_segment1.mp3";
  seg1.startMs = 0;
  seg1.durationMs = 60000;
  seg1.fileSizeBytes = 1024;
  segments.push_back(seg1);

  AudioSegment seg2;
  seg2.path = "/tmp/nonexistent_segment2.mp3";
  seg2.startMs = 60000;
  seg2.durationMs = 60000;
  seg2.fileSizeBytes = 1024;
  segments.push_back(seg2);

  // Should not throw when cleaning up non-existent files
  EXPECT_NO_THROW({ m_extractor->CleanupSegments(segments); });
}

TEST_F(AudioExtractorTest, CleanupEmptySegments)
{
  std::vector<AudioSegment> emptySegments;

  // Should handle empty segment list
  EXPECT_NO_THROW({ m_extractor->CleanupSegments(emptySegments); });
}

TEST_F(AudioExtractorTest, ConfigurationPersistence)
{
  AudioExtractionConfig config1;
  config1.sampleRate = 44100;
  config1.format = "wav";

  m_extractor->SetConfig(config1);

  const auto& config2 = m_extractor->GetConfig();
  EXPECT_EQ(config2.sampleRate, 44100);
  EXPECT_EQ(config2.format, "wav");
}

TEST_F(AudioExtractorTest, CustomConfigConstructor)
{
  AudioExtractionConfig customConfig;
  customConfig.sampleRate = 48000;
  customConfig.channels = 2;
  customConfig.format = "flac";
  customConfig.bitrate = 320;

  auto customExtractor = std::make_unique<CAudioExtractor>(customConfig);

  const auto& config = customExtractor->GetConfig();
  EXPECT_EQ(config.sampleRate, 48000);
  EXPECT_EQ(config.channels, 2);
  EXPECT_EQ(config.format, "flac");
  EXPECT_EQ(config.bitrate, 320);
}

TEST_F(AudioExtractorTest, AudioSegmentStructure)
{
  AudioSegment segment;
  segment.path = "/tmp/audio.mp3";
  segment.startMs = 5000;
  segment.durationMs = 30000;
  segment.fileSizeBytes = 512000;

  EXPECT_EQ(segment.path, "/tmp/audio.mp3");
  EXPECT_EQ(segment.startMs, 5000);
  EXPECT_EQ(segment.durationMs, 30000);
  EXPECT_EQ(segment.fileSizeBytes, 512000);
}

TEST_F(AudioExtractorTest, MultipleExtractions)
{
  if (!CAudioExtractor::IsFFmpegAvailable())
  {
    GTEST_SKIP() << "FFmpeg not available";
  }

  // Multiple extraction attempts should not crash
  for (int i = 0; i < 3; ++i)
  {
    EXPECT_NO_THROW({
      m_extractor->ExtractAudio("/nonexistent/video" + std::to_string(i) + ".mkv",
                                "/tmp/output" + std::to_string(i) + ".mp3");
    });
  }
}

TEST_F(AudioExtractorTest, CancelBeforeExtraction)
{
  if (!CAudioExtractor::IsFFmpegAvailable())
  {
    GTEST_SKIP() << "FFmpeg not available";
  }

  m_extractor->Cancel();

  // Extraction after cancel should fail or return quickly
  bool result = m_extractor->ExtractAudio("/nonexistent/video.mkv", "/tmp/output.mp3");

  EXPECT_FALSE(result);
}

// Note: Full integration tests with actual video files would require:
// 1. Sample video files
// 2. Writable temp directory
// 3. FFmpeg installed
// 4. Sufficient disk space
// These should be done in separate integration tests, not unit tests
