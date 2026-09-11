/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDSubtitles/SubtitlesStyle.h"

#include <gtest/gtest.h>

using namespace KODI::SUBTITLES::STYLE;

namespace
{

//! \brief A 16:9 video filling a 16:9 screen, which is the case that must not move.
renderOpts FullScreen()
{
  renderOpts opts;
  opts.frameWidth = 1920.0f;
  opts.frameHeight = 1080.0f;
  opts.videoWidth = 1920.0f;
  opts.videoHeight = 1080.0f;
  return opts;
}

} // unnamed namespace

TEST(TestSubtitleMargins, NoInsetWhenTheVideoFillsTheFrame)
{
  const FrameMargins margins = InsideVideoMargins(FullScreen());

  EXPECT_EQ(margins.top, 0);
  EXPECT_EQ(margins.left, 0);
}

TEST(TestSubtitleMargins, InsetsToTheVideoWhenThePictureIsUnknown)
{
  // A 4:3 video pillarboxed by the player into a 16:9 screen. The bars are the player's, not
  // the content's, and this is the behaviour that existed before a content rectangle did.
  renderOpts opts = FullScreen();
  opts.videoWidth = 1440.0f;

  const FrameMargins margins = InsideVideoMargins(opts);

  EXPECT_EQ(margins.top, 0);
  EXPECT_EQ(margins.left, 240);
}

TEST(TestSubtitleMargins, InsetsToThePictureNotTheCodedFrame)
{
  // A 2.35:1 film in a 16:9 frame, filling the screen. The bars are coded into the video, so
  // the video is the whole 1080 - inset to that and subtitles land in the letterbox.
  renderOpts opts = FullScreen();
  opts.contentWidth = 1920.0f;
  opts.contentHeight = 816.0f;

  const FrameMargins margins = InsideVideoMargins(opts);

  EXPECT_EQ(margins.top, 132);
  EXPECT_EQ(margins.left, 0);
}

TEST(TestSubtitleMargins, InsetsToThePictureOfAPillarboxedTitle)
{
  // 4:3 archive content coded into a 16:9 frame, the case where side masking matters most.
  renderOpts opts = FullScreen();
  opts.contentWidth = 1440.0f;
  opts.contentHeight = 1080.0f;

  const FrameMargins margins = InsideVideoMargins(opts);

  EXPECT_EQ(margins.top, 0);
  EXPECT_EQ(margins.left, 240);
}

TEST(TestSubtitleMargins, CombinesThePlayersBarsWithTheContentsOwn)
{
  // A scope film shown in a window taller than it needs: the player letterboxes it, and the
  // content is letterboxed inside that. Both insets are real and the text belongs inside both.
  renderOpts opts = FullScreen();
  opts.videoWidth = 1440.0f;
  opts.videoHeight = 810.0f;
  opts.contentWidth = 1440.0f;
  opts.contentHeight = 612.0f;

  const FrameMargins margins = InsideVideoMargins(opts);

  EXPECT_EQ(margins.top, 234);
  EXPECT_EQ(margins.left, 240);
}

TEST(TestSubtitleMargins, NeverInsetsOutwardWhenZoomedBeyondTheFrame)
{
  // Zoom can push the picture past the edges of the screen. A negative margin would place text
  // off-screen, so the inset clamps at the frame.
  renderOpts opts = FullScreen();
  opts.videoWidth = 2400.0f;
  opts.videoHeight = 1350.0f;
  opts.contentWidth = 2400.0f;
  opts.contentHeight = 1200.0f;

  const FrameMargins margins = InsideVideoMargins(opts);

  EXPECT_EQ(margins.top, 0);
  EXPECT_EQ(margins.left, 0);
}

TEST(TestSubtitleMargins, AContentRectMatchingTheVideoChangesNothing)
{
  // Uncropped content: the measurement agrees with the coded frame, and must be indistinguishable
  // from having no measurement at all.
  renderOpts opts = FullScreen();
  opts.videoWidth = 1440.0f;
  opts.contentWidth = 1440.0f;
  opts.contentHeight = 1080.0f;

  const FrameMargins margins = InsideVideoMargins(opts);

  EXPECT_EQ(margins.top, 0);
  EXPECT_EQ(margins.left, 240);
}
