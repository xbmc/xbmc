/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/ingest/SubtitleParser.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI::SEMANTIC;

class SubtitleParserTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_parser = std::make_unique<CSubtitleParser>();
    m_tempDir = "special://temp/semantic_tests/";
    XFILE::CDirectory::Create(m_tempDir);
  }

  void TearDown() override { XFILE::CDirectory::RemoveRecursive(m_tempDir); }

  std::string CreateTempFile(const std::string& filename, const std::string& content)
  {
    std::string path = URIUtils::AddFileToFolder(m_tempDir, filename);
    std::string translatedPath = CSpecialProtocol::TranslatePath(path);

    XFILE::CFile file;
    if (file.OpenForWrite(translatedPath))
    {
      file.Write(content.c_str(), content.length());
      file.Close();
    }

    return translatedPath;
  }

  std::unique_ptr<CSubtitleParser> m_parser;
  std::string m_tempDir;
};

TEST_F(SubtitleParserTest, ParseSRTTimestamp)
{
  std::string srtContent = R"(1
00:00:01,000 --> 00:00:04,500
Hello, world!

2
00:01:30,500 --> 00:01:35,000
This is a test.
)";

  std::string tempPath = CreateTempFile("test.srt", srtContent);

  auto entries = m_parser->Parse(tempPath);
  ASSERT_EQ(entries.size(), 2);

  EXPECT_EQ(entries[0].startMs, 1000);
  EXPECT_EQ(entries[0].endMs, 4500);
  EXPECT_EQ(entries[0].text, "Hello, world!");

  EXPECT_EQ(entries[1].startMs, 90500); // 1:30.500
  EXPECT_EQ(entries[1].endMs, 95000);   // 1:35.000
  EXPECT_EQ(entries[1].text, "This is a test.");
}

TEST_F(SubtitleParserTest, StripHTMLTags)
{
  std::string srtContent = R"(1
00:00:01,000 --> 00:00:04,000
This is <i>italic</i> and <b>bold</b> text.
)";

  std::string tempPath = CreateTempFile("test.srt", srtContent);

  auto entries = m_parser->Parse(tempPath);
  ASSERT_EQ(entries.size(), 1);
  EXPECT_EQ(entries[0].text, "This is italic and bold text.");
}

TEST_F(SubtitleParserTest, FilterNonDialogue)
{
  std::string srtContent = R"(1
00:00:01,000 --> 00:00:04,000
[music]

2
00:00:05,000 --> 00:00:08,000
♪ La la la ♪

3
00:00:10,000 --> 00:00:15,000
This is actual dialogue.
)";

  std::string tempPath = CreateTempFile("test.srt", srtContent);

  auto entries = m_parser->Parse(tempPath);
  // Non-dialogue should be filtered
  EXPECT_GE(entries.size(), 1);

  // Find the dialogue entry
  bool foundDialogue = false;
  for (const auto& entry : entries)
  {
    if (entry.text.find("actual dialogue") != std::string::npos)
    {
      foundDialogue = true;
      break;
    }
  }
  EXPECT_TRUE(foundDialogue);
}

TEST_F(SubtitleParserTest, ParseASS)
{
  std::string assContent = R"([Script Info]
Title: Test

[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
Dialogue: 0,0:00:01.00,0:00:04.50,Default,,0,0,0,,Hello world
Dialogue: 0,0:00:05.00,0:00:08.20,Speaker1,,0,0,0,,{\an8}Test with codes
)";

  std::string tempPath = CreateTempFile("test.ass", assContent);

  auto entries = m_parser->Parse(tempPath);
  ASSERT_GE(entries.size(), 2);

  EXPECT_EQ(entries[0].startMs, 1000);
  EXPECT_EQ(entries[0].text, "Hello world");

  // ASS codes should be stripped
  bool foundTestEntry = false;
  for (const auto& entry : entries)
  {
    if (entry.text.find("Test with codes") != std::string::npos)
    {
      foundTestEntry = true;
      EXPECT_EQ(entry.text.find("{\\an8}"), std::string::npos); // Codes stripped
      break;
    }
  }
  EXPECT_TRUE(foundTestEntry);
}

TEST_F(SubtitleParserTest, ParseVTT)
{
  std::string vttContent = R"(WEBVTT

00:00:01.000 --> 00:00:04.500
Hello, world!

00:01:30.500 --> 00:01:35.000
This is WebVTT.
)";

  std::string tempPath = CreateTempFile("test.vtt", vttContent);

  auto entries = m_parser->Parse(tempPath);
  ASSERT_EQ(entries.size(), 2);

  EXPECT_EQ(entries[0].startMs, 1000);
  EXPECT_EQ(entries[0].endMs, 4500);
  EXPECT_EQ(entries[0].text, "Hello, world!");

  EXPECT_EQ(entries[1].startMs, 90500);
  EXPECT_EQ(entries[1].text, "This is WebVTT.");
}

TEST_F(SubtitleParserTest, MultiLineEntries)
{
  std::string srtContent = R"(1
00:00:01,000 --> 00:00:04,000
This is a multi-line
subtitle entry.

2
00:00:05,000 --> 00:00:08,000
Another one
with two lines.
)";

  std::string tempPath = CreateTempFile("test.srt", srtContent);

  auto entries = m_parser->Parse(tempPath);
  ASSERT_EQ(entries.size(), 2);

  // Multi-line text should be joined with spaces
  EXPECT_NE(entries[0].text.find("multi-line"), std::string::npos);
  EXPECT_NE(entries[0].text.find("subtitle entry"), std::string::npos);
}

TEST_F(SubtitleParserTest, CanParseSRT)
{
  EXPECT_TRUE(m_parser->CanParse("test.srt"));
  EXPECT_TRUE(m_parser->CanParse("test.SRT"));
  EXPECT_TRUE(m_parser->CanParse("/path/to/movie.srt"));
}

TEST_F(SubtitleParserTest, CanParseASS)
{
  EXPECT_TRUE(m_parser->CanParse("test.ass"));
  EXPECT_TRUE(m_parser->CanParse("test.ssa"));
  EXPECT_TRUE(m_parser->CanParse("test.ASS"));
}

TEST_F(SubtitleParserTest, CanParseVTT)
{
  EXPECT_TRUE(m_parser->CanParse("test.vtt"));
  EXPECT_TRUE(m_parser->CanParse("test.VTT"));
}

TEST_F(SubtitleParserTest, CannotParseUnsupported)
{
  EXPECT_FALSE(m_parser->CanParse("test.txt"));
  EXPECT_FALSE(m_parser->CanParse("test.sub"));
  EXPECT_FALSE(m_parser->CanParse("test.mkv"));
}

TEST_F(SubtitleParserTest, GetSupportedExtensions)
{
  auto extensions = m_parser->GetSupportedExtensions();

  EXPECT_GE(extensions.size(), 3);
  EXPECT_NE(std::find(extensions.begin(), extensions.end(), "srt"), extensions.end());
  EXPECT_NE(std::find(extensions.begin(), extensions.end(), "ass"), extensions.end());
  EXPECT_NE(std::find(extensions.begin(), extensions.end(), "vtt"), extensions.end());
}

TEST_F(SubtitleParserTest, EmptyFile)
{
  std::string tempPath = CreateTempFile("empty.srt", "");

  auto entries = m_parser->Parse(tempPath);
  EXPECT_TRUE(entries.empty());
}

TEST_F(SubtitleParserTest, MalformedTimestamp)
{
  std::string srtContent = R"(1
invalid --> timestamp
This should not parse.

2
00:00:05,000 --> 00:00:08,000
This should parse.
)";

  std::string tempPath = CreateTempFile("test.srt", srtContent);

  auto entries = m_parser->Parse(tempPath);

  // Should skip malformed entry but parse valid one
  bool foundValid = false;
  for (const auto& entry : entries)
  {
    if (entry.text.find("This should parse") != std::string::npos)
    {
      foundValid = true;
      EXPECT_EQ(entry.startMs, 5000);
      break;
    }
  }
  EXPECT_TRUE(foundValid);
}

TEST_F(SubtitleParserTest, UTF8Content)
{
  std::string srtContent = R"(1
00:00:01,000 --> 00:00:04,000
Hello 世界 Привет мир
)";

  std::string tempPath = CreateTempFile("test.srt", srtContent);

  auto entries = m_parser->Parse(tempPath);
  ASSERT_EQ(entries.size(), 1);

  // UTF-8 content should be preserved
  EXPECT_NE(entries[0].text.find("世界"), std::string::npos);
  EXPECT_NE(entries[0].text.find("Привет"), std::string::npos);
}

TEST_F(SubtitleParserTest, ConfidenceDefault)
{
  std::string srtContent = R"(1
00:00:01,000 --> 00:00:04,000
Test subtitle
)";

  std::string tempPath = CreateTempFile("test.srt", srtContent);

  auto entries = m_parser->Parse(tempPath);
  ASSERT_EQ(entries.size(), 1);

  // Subtitles should have confidence 1.0
  EXPECT_FLOAT_EQ(entries[0].confidence, 1.0f);
}
