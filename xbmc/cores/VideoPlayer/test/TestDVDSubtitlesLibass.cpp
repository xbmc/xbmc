/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDSubtitles/DVDSubtitlesLibass.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"

#include <algorithm>
#include <memory>

#include <gtest/gtest.h>

using namespace KODI::SUBTITLES::STYLE;

namespace
{

constexpr float FRAME_HEIGHT = 1080.0f;

//! \brief An adapted track, as a SubRip stream has, with one line on screen from 10s to 20s.
class CLibassWithOneLine : public CDVDSubtitlesLibass
{
public:
  CLibassWithOneLine()
  {
    Configure();
    SetSubtitleType(ADAPTED);
    CreateTrack();
    CreateStyle();
    AddEvent("Line", DVD_SEC_TO_TIME(10), DVD_SEC_TO_TIME(20));
  }

  //! \brief The top edge of what is rendered at a time, or -1 when nothing is on screen.
  int RenderedTop(double seconds, const std::shared_ptr<style>& subStyle)
  {
    renderOpts opts;
    opts.frameWidth = 1920.0f;
    opts.frameHeight = FRAME_HEIGHT;
    opts.videoWidth = opts.frameWidth;
    opts.videoHeight = opts.frameHeight;
    opts.sourceWidth = opts.frameWidth;
    opts.sourceHeight = opts.frameHeight;
    opts.m_par = 1.0f;

    const ASS_Image* image = RenderImage(DVD_SEC_TO_TIME(seconds), opts, subStyle);
    if (!image)
      return -1;

    int top = image->dst_y;
    for (; image; image = image->next)
      top = std::min(top, image->dst_y);
    return top;
  }
};

std::shared_ptr<style> Aligned(FontAlign alignment)
{
  auto subStyle = std::make_shared<style>();
  subStyle->fontSize = 40.0;
  subStyle->alignment = alignment;
  return subStyle;
}

} // unnamed namespace

TEST(TestDVDSubtitlesLibass, AStyleChangedWithNothingOnScreenPlacesTheNextLine)
{
  CLibassWithOneLine libass;
  ASSERT_EQ(libass.RenderedTop(0, Aligned(FontAlign::TOP_CENTER)), -1);

  const std::shared_ptr<style> bottom{Aligned(FontAlign::SUB_CENTER)};
  ASSERT_EQ(libass.RenderedTop(5, bottom), -1);

  EXPECT_GT(libass.RenderedTop(15, bottom), FRAME_HEIGHT / 2);
}

TEST(TestDVDSubtitlesLibass, AStyleIsAppliedOnceNotOnEveryFrame)
{
  CLibassWithOneLine libass;
  const std::shared_ptr<style> subStyle{Aligned(FontAlign::TOP_CENTER)};
  ASSERT_EQ(libass.RenderedTop(0, subStyle), -1);

  // Only a new style object is a new style, so an edit in place is never applied.
  subStyle->alignment = FontAlign::SUB_CENTER;

  const int top{libass.RenderedTop(15, subStyle)};
  ASSERT_GE(top, 0) << "nothing was rendered, so the placement was not tested";
  EXPECT_LT(top, FRAME_HEIGHT / 2);
}
