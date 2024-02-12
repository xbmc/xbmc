/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <cmath>
#include <gtest/gtest.h>
#include "test/TestUtils.h"
#include "cores/VideoPlayer/DVDSubtitles/webvtt/WebVTTHandler.h"
#include "cores/VideoPlayer/DVDSubtitles/SubtitleParserWebVTT.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"


/*
 * CSubtitleParserWebVTT will build up a subtitle stream.
 * If CMAKE has ENABLE_OPTICAL it may blow up on MediaManager
 * attempting to compare CDDrive paths.
 * Disable ENABLE_OPTICAL to build CSubtitleParserWebVTT
 * standalone without starting up entire ServiceManager
 * architechture.
 */

TEST(TestWebVTT, TestWebVTTSignature_Minimal) {
  std::string path = XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/DVDSubtitles/test/webvtt/testdata/sample1_minimal.vtt");
  CSubtitleParserWebVTT parserWebVtt = CSubtitleParserWebVTT(nullptr, path);
  CDVDStreamInfo hints = CDVDStreamInfo();
  EXPECT_EQ(true, parserWebVtt.Open(hints));
}

TEST(TestWebVTT, TestWebVTTSignature_Text_Space_LF) {
  std::string path = XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/DVDSubtitles/test/webvtt/testdata/sample2_text_space_lf.vtt");
  CSubtitleParserWebVTT parserWebVtt = CSubtitleParserWebVTT(nullptr, path);
  CDVDStreamInfo hints = CDVDStreamInfo();
  EXPECT_EQ(true, parserWebVtt.Open(hints));
}

TEST(TestWebVTT, TestWebVTTSignature_Text_Tab_LF) {
  std::string path = XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/DVDSubtitles/test/webvtt/testdata/sample3_text_tab_lf.vtt");
  CSubtitleParserWebVTT parserWebVtt = CSubtitleParserWebVTT(nullptr, path);
  CDVDStreamInfo hints = CDVDStreamInfo();
  EXPECT_EQ(true, parserWebVtt.Open(hints));
}

TEST(TestWebVTT, TestWebVTTSignature_Text_Space_CRLF) {
  std::string path = XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/DVDSubtitles/test/webvtt/testdata/sample4_text_space_crlf.vtt");
  CSubtitleParserWebVTT parserWebVtt = CSubtitleParserWebVTT(nullptr, path);
  CDVDStreamInfo hints = CDVDStreamInfo();
  EXPECT_EQ(true, parserWebVtt.Open(hints));
}

TEST(TestWebVTT, TestWebVTTSignature_Text_Tab_CRLF) {
  std::string path = XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/DVDSubtitles/test/webvtt/testdata/sample5_text_tab_crlf.vtt");
  CSubtitleParserWebVTT parserWebVtt = CSubtitleParserWebVTT(nullptr, path);
  CDVDStreamInfo hints = CDVDStreamInfo();
  EXPECT_EQ(true, parserWebVtt.Open(hints));
}

TEST(TestWebVTT, TestWebVTTSignature_Text_Space_CR_No_LF) {
  std::string path = XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/DVDSubtitles/test/webvtt/testdata/sample6_text_space_crcr.vtt");
  CSubtitleParserWebVTT parserWebVtt = CSubtitleParserWebVTT(nullptr, path);
  CDVDStreamInfo hints = CDVDStreamInfo();
  EXPECT_EQ(true, parserWebVtt.Open(hints));
}

TEST(TestWebVTT, TestWebVTTSignature_BOM) {
  std::string path = XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/DVDSubtitles/test/webvtt/testdata/sample7_minimal_BOM.vtt");
  CSubtitleParserWebVTT parserWebVtt = CSubtitleParserWebVTT(nullptr, path);
  CDVDStreamInfo hints = CDVDStreamInfo();
  EXPECT_EQ(true, parserWebVtt.Open(hints));
}
