/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "windowing/GraphicContext.h"
#include "windowing/Resolution.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
constexpr float FPS_23_976{24000.0f / 1001.0f};
constexpr float HZ_59_94{60000.0f / 1001.0f};
constexpr float HZ_119_88{120000.0f / 1001.0f};

class TestResolutionWhitelist : public ::testing::Test
{
protected:
  //! Add a display mode to the user whitelist
  RESOLUTION Whitelist(int width, int height, float refreshRate, uint32_t flags = 0)
  {
    return Add(m_whitelist, width, height, refreshRate, flags);
  }

  //! Add a display mode the display allows, used to build the default whitelist
  RESOLUTION Allow(int width, int height, float refreshRate, uint32_t flags = 0)
  {
    return Add(m_allowed, width, height, refreshRate, flags);
  }

  //! Set the desktop display mode, which is also the display mode currently chosen for the video
  void Desktop(int width, int height, float refreshRate, uint32_t flags = 0)
  {
    m_desktop = Info(width, height, refreshRate, flags);
    m_current = m_desktop;
  }

  RESOLUTION Select(float fps, int width = 1920, int height = 1080) const
  {
    return CResolutionUtils::SelectFromWhitelist(fps, width, height, m_current, m_desktop,
                                                 m_whitelist, m_allowed, m_options);
  }

  WhitelistOptions m_options;
  RESOLUTION_INFO m_current{Info(1920, 1080, 60.0f)};
  RESOLUTION_INFO m_desktop{Info(1920, 1080, 60.0f)};

private:
  static RESOLUTION_INFO Info(int width, int height, float refreshRate, uint32_t flags = 0)
  {
    RESOLUTION_INFO info(width, height, 0.0f,
                         std::to_string(width) + "x" + std::to_string(height) + "@" +
                             std::to_string(refreshRate));
    info.fRefreshRate = refreshRate;
    info.dwFlags = flags;
    return info;
  }

  RESOLUTION Add(std::vector<ResolutionCandidate>& modes,
                 int width,
                 int height,
                 float refreshRate,
                 uint32_t flags)
  {
    const auto res = static_cast<RESOLUTION>(m_next++);
    modes.push_back({res, Info(width, height, refreshRate, flags)});
    return res;
  }

  std::vector<ResolutionCandidate> m_whitelist;
  std::vector<ResolutionCandidate> m_allowed;
  int m_next{RES_CUSTOM};
};
} // namespace

// Refresh rates

TEST_F(TestResolutionWhitelist, ExactRefreshRateIsChosen)
{
  Whitelist(1920, 1080, 60.0f);
  const RESOLUTION exact = Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f), exact);
}

TEST_F(TestResolutionWhitelist, FractionalAndIntegerRefreshRatesAreDistinguished)
{
  const RESOLUTION fractional = Whitelist(1920, 1080, FPS_23_976);
  const RESOLUTION integer = Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(FPS_23_976), fractional);
  EXPECT_EQ(Select(24.0f), integer);
}

TEST_F(TestResolutionWhitelist, ResolutionWithoutPermittedRefreshRateIsNotChosen)
{
  Whitelist(1920, 1080, 60.0f);
  Whitelist(1920, 1080, 50.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);
}

TEST_F(TestResolutionWhitelist, DisabledAlternativeRefreshRatesAreNotChosen)
{
  Whitelist(1920, 1080, 48.0f);
  Whitelist(1920, 1080, 60.0f);
  Whitelist(1920, 1080, 120.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);
}

TEST_F(TestResolutionWhitelist, DoubleRefreshRateIsChosenWhenAllowed)
{
  const RESOLUTION doubled = Whitelist(1920, 1080, 48.0f);
  Whitelist(1920, 1080, 60.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);

  m_options.allowDoubleRefreshRate = true;
  EXPECT_EQ(Select(24.0f), doubled);
}

TEST_F(TestResolutionWhitelist, ExactRefreshRateBeatsDoubleWithoutMultipleRefreshRates)
{
  m_options.allowDoubleRefreshRate = true;
  Whitelist(1920, 1080, 48.0f);
  const RESOLUTION exact = Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f), exact);
}

TEST_F(TestResolutionWhitelist, PulldownRefreshRateIsChosenWhenAllowed)
{
  const RESOLUTION pulldown = Whitelist(1920, 1080, 60.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);

  m_options.allowPulldown = true;
  EXPECT_EQ(Select(24.0f), pulldown);
}

TEST_F(TestResolutionWhitelist, FractionalPulldownRefreshRateIsChosen)
{
  m_options.allowPulldown = true;
  Whitelist(1920, 1080, 60.0f);
  const RESOLUTION pulldown = Whitelist(1920, 1080, HZ_59_94);

  EXPECT_EQ(Select(FPS_23_976), pulldown);
}

TEST_F(TestResolutionWhitelist, DoubleRefreshRateBeatsPulldown)
{
  m_options.allowDoubleRefreshRate = true;
  m_options.allowPulldown = true;
  Whitelist(1920, 1080, 60.0f);
  const RESOLUTION doubled = Whitelist(1920, 1080, 48.0f);

  EXPECT_EQ(Select(24.0f), doubled);
}

TEST_F(TestResolutionWhitelist, MultipleRefreshRateIsChosenWhenAllowed)
{
  m_options.allowDoubleRefreshRate = true;
  const RESOLUTION multiple = Whitelist(1920, 1080, 120.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);

  m_options.useMultipleRefreshRates = true;
  EXPECT_EQ(Select(24.0f), multiple);
}

TEST_F(TestResolutionWhitelist, MultipleRefreshRatesNeedDoubleRefreshRatesAllowed)
{
  m_options.useMultipleRefreshRates = true;
  Whitelist(1920, 1080, 120.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);
}

TEST_F(TestResolutionWhitelist, MultipleRefreshRatesPreferHigherRefreshRates)
{
  m_options.allowDoubleRefreshRate = true;
  m_options.useMultipleRefreshRates = true;
  Whitelist(1920, 1080, 24.0f);
  const RESOLUTION doubled = Whitelist(1920, 1080, 48.0f);

  EXPECT_EQ(Select(24.0f), doubled);

  const RESOLUTION multiple = Whitelist(1920, 1080, 120.0f);
  EXPECT_EQ(Select(24.0f), multiple);
}

TEST_F(TestResolutionWhitelist, HighestMultipleRefreshRateIsChosen)
{
  m_options.allowDoubleRefreshRate = true;
  m_options.useMultipleRefreshRates = true;
  Whitelist(1920, 1080, 120.0f);
  const RESOLUTION highest = Whitelist(1920, 1080, 144.0f);
  Whitelist(1920, 1080, 72.0f);

  EXPECT_EQ(Select(24.0f), highest);
}

TEST_F(TestResolutionWhitelist, MultipleRefreshRateMustMatchFractionalFrameRate)
{
  m_options.allowDoubleRefreshRate = true;
  m_options.useMultipleRefreshRates = true;
  Whitelist(1920, 1080, 120.0f);

  EXPECT_EQ(Select(FPS_23_976), RES_INVALID);

  const RESOLUTION fractional = Whitelist(1920, 1080, HZ_119_88);
  EXPECT_EQ(Select(FPS_23_976), fractional);
}

TEST_F(TestResolutionWhitelist, LowerRefreshRatesAreNotMultiples)
{
  m_options.allowDoubleRefreshRate = true;
  m_options.useMultipleRefreshRates = true;
  m_options.allowPulldown = true;
  Whitelist(1920, 1080, 25.0f);

  EXPECT_EQ(Select(50.0f), RES_INVALID);
}

TEST_F(TestResolutionWhitelist, PulldownMultiplesNeedMultipleRefreshRates)
{
  m_options.allowDoubleRefreshRate = true;
  m_options.allowPulldown = true;
  const RESOLUTION pulldownMultiple = Whitelist(1920, 1080, 180.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);

  m_options.useMultipleRefreshRates = true;
  EXPECT_EQ(Select(24.0f), pulldownMultiple);
}

// Resolutions

TEST_F(TestResolutionWhitelist, VideoResolutionBeatsDesktopResolution)
{
  Desktop(3840, 2160, 60.0f);
  Whitelist(3840, 2160, 24.0f);
  const RESOLUTION video = Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f), video);
}

TEST_F(TestResolutionWhitelist, ResolutionOnlyMatchCannotBeatRefreshRateMatch)
{
  Desktop(3840, 2160, 60.0f);
  Whitelist(1920, 1080, 60.0f);
  const RESOLUTION desktop = Whitelist(3840, 2160, 24.0f);

  EXPECT_EQ(Select(24.0f), desktop);
}

TEST_F(TestResolutionWhitelist, VideoResolutionIsPreferredOverABetterRefreshRate)
{
  m_options.allowPulldown = true;
  Desktop(3840, 2160, 60.0f);
  const RESOLUTION video = Whitelist(1920, 1080, 60.0f);
  Whitelist(3840, 2160, 24.0f);

  EXPECT_EQ(Select(24.0f), video);
}

TEST_F(TestResolutionWhitelist, DesktopResolutionBeatsOtherResolutions)
{
  Desktop(3840, 2160, 60.0f);
  Whitelist(2560, 1440, 24.0f);
  const RESOLUTION desktop = Whitelist(3840, 2160, 24.0f);

  EXPECT_EQ(Select(24.0f), desktop);
}

TEST_F(TestResolutionWhitelist, OtherResolutionIsChosenWhenNothingElseMatches)
{
  Desktop(3840, 2160, 60.0f);
  const RESOLUTION other = Whitelist(2560, 1440, 24.0f);

  EXPECT_EQ(Select(24.0f), other);
}

TEST_F(TestResolutionWhitelist, ClosestResolutionWinsAtEqualWeighting)
{
  Desktop(1280, 720, 60.0f);
  Whitelist(3840, 2160, 24.0f);
  const RESOLUTION closest = Whitelist(2560, 1440, 24.0f);

  EXPECT_EQ(Select(24.0f), closest);
}

TEST_F(TestResolutionWhitelist, ExactSizeBeatsSmallerModeWithinTolerance)
{
  Whitelist(1920, 1064, 24.0f);
  const RESOLUTION exact = Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f), exact);
}

// Video size tolerance

TEST_F(TestResolutionWhitelist, DecoderPaddedVideoMatchesMode)
{
  Desktop(3840, 2160, 60.0f);
  Whitelist(3840, 2160, 24.0f);
  const RESOLUTION video = Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f, 1920, 1088), video);
}

TEST_F(TestResolutionWhitelist, PaddingBeyondToleranceDoesNotMatchMode)
{
  Desktop(3840, 2160, 60.0f);
  const RESOLUTION desktop = Whitelist(3840, 2160, 24.0f);
  Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f, 1920, 1120), desktop);
}

TEST_F(TestResolutionWhitelist, LetterboxedVideoMatchesFullWidthMode)
{
  Desktop(3840, 2160, 60.0f);
  Whitelist(3840, 2160, 24.0f);
  const RESOLUTION video = Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f, 1920, 800), video);
}

TEST_F(TestResolutionWhitelist, PillarboxedVideoMatchesFullHeightMode)
{
  Desktop(3840, 2160, 60.0f);
  Whitelist(3840, 2160, 24.0f);
  const RESOLUTION video = Whitelist(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f, 1440, 1080), video);
}

TEST_F(TestResolutionWhitelist, EncoderCroppedVideoMatchesMode)
{
  Whitelist(1920, 1080, 24.0f);
  const RESOLUTION video = Whitelist(3840, 2160, 24.0f);

  EXPECT_EQ(Select(24.0f, 3832, 1600), video);
}

TEST_F(TestResolutionWhitelist, SmallerVideoDoesNotMatchLargerMode)
{
  Desktop(3840, 2160, 60.0f);
  Whitelist(1920, 1080, 24.0f);
  const RESOLUTION desktop = Whitelist(3840, 2160, 24.0f);

  EXPECT_EQ(Select(24.0f, 1280, 720), desktop);
}

// Mode flags

TEST_F(TestResolutionWhitelist, ModesWithADifferentScanTypeAreIgnored)
{
  Desktop(3840, 2160, 60.0f);
  Whitelist(1920, 1080, 24.0f, D3DPRESENTFLAG_INTERLACED);

  EXPECT_EQ(Select(24.0f), RES_INVALID);

  const RESOLUTION progressive = Whitelist(3840, 2160, 24.0f);
  EXPECT_EQ(Select(24.0f), progressive);
}

TEST_F(TestResolutionWhitelist, StereoscopicModeMustMatchCurrentMode)
{
  Whitelist(1920, 1080, 24.0f);
  const RESOLUTION stereo = Whitelist(1920, 1080, 24.0f, D3DPRESENTFLAG_MODE3DSBS);
  m_current.dwFlags = D3DPRESENTFLAG_MODE3DSBS;

  EXPECT_EQ(Select(24.0f), stereo);
}

TEST_F(TestResolutionWhitelist, FlagsOutsideTheModeMaskAreIgnored)
{
  const RESOLUTION mode =
      Whitelist(1920, 1080, 24.0f, D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN);

  EXPECT_EQ(Select(24.0f), mode);
}

// Default whitelist

TEST_F(TestResolutionWhitelist, EmptyWhitelistAllowsDoubleRefreshRate)
{
  const RESOLUTION doubled = Allow(1920, 1080, 48.0f);

  EXPECT_EQ(Select(24.0f), doubled);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistAllowsPulldownRefreshRate)
{
  const RESOLUTION pulldown = Allow(1920, 1080, 60.0f);

  EXPECT_EQ(Select(24.0f), pulldown);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistNeedsMultipleRefreshRatesEnabled)
{
  const RESOLUTION exact = Allow(1920, 1080, 24.0f);
  const RESOLUTION multiple = Allow(1920, 1080, 120.0f);

  EXPECT_EQ(Select(24.0f), exact);

  m_options.useMultipleRefreshRates = true;
  EXPECT_EQ(Select(24.0f), multiple);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistSkipsHalfRefreshRates)
{
  Allow(1920, 1080, 25.0f);
  const RESOLUTION fifty = Allow(1920, 1080, 50.0f);
  Allow(1920, 1080, 30.0f);
  const RESOLUTION sixty = Allow(1920, 1080, 60.0f);

  EXPECT_EQ(Select(25.0f), fifty);
  EXPECT_EQ(Select(30.0f), sixty);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistKeepsTwentyFourHertz)
{
  const RESOLUTION fractional = Allow(1920, 1080, FPS_23_976);
  const RESOLUTION integer = Allow(1920, 1080, 24.0f);

  EXPECT_EQ(Select(FPS_23_976), fractional);
  EXPECT_EQ(Select(24.0f), integer);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistExcludesModesSmallerThanCurrentMode)
{
  Desktop(3840, 2160, 60.0f);
  Allow(1920, 1080, 24.0f);
  const RESOLUTION desktop = Allow(3840, 2160, 24.0f);

  EXPECT_EQ(Select(24.0f), desktop);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistMatchingResolutionAllowsVideoSizedModes)
{
  m_options.matchResolution = true;
  Desktop(3840, 2160, 60.0f);
  Allow(3840, 2160, 24.0f);
  const RESOLUTION video = Allow(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f), video);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistMatchingResolutionExcludesModesSmallerThanVideo)
{
  m_options.matchResolution = true;
  Desktop(3840, 2160, 60.0f);
  Allow(1280, 720, 24.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistMatchingResolutionAllowsLargerModes)
{
  m_options.matchResolution = true;
  const RESOLUTION larger = Allow(3840, 2160, 24.0f);

  EXPECT_EQ(Select(24.0f), larger);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistMatchingResolutionKeepsModesForPaddedVideo)
{
  m_options.matchResolution = true;
  Desktop(3840, 2160, 60.0f);
  const RESOLUTION hd = Allow(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f, 1920, 1088), hd);

  const RESOLUTION uhd = Allow(3840, 2160, 24.0f);
  EXPECT_EQ(Select(24.0f, 3840, 2176), uhd);
}

TEST_F(TestResolutionWhitelist, EmptyWhitelistExcludesModesWithADifferentScanType)
{
  Allow(1920, 1080, 24.0f, D3DPRESENTFLAG_INTERLACED);

  EXPECT_EQ(Select(24.0f), RES_INVALID);
}

TEST_F(TestResolutionWhitelist, NoModesMatchNothing)
{
  EXPECT_EQ(Select(24.0f), RES_INVALID);
}

// Explicit whitelist

TEST_F(TestResolutionWhitelist, ExplicitWhitelistIgnoresAllowedModes)
{
  Whitelist(1920, 1080, 60.0f);
  Allow(1920, 1080, 24.0f);

  EXPECT_EQ(Select(24.0f), RES_INVALID);
}

TEST_F(TestResolutionWhitelist, ExplicitWhitelistIsNotFilteredByResolution)
{
  Desktop(3840, 2160, 60.0f);
  const RESOLUTION smaller = Whitelist(1280, 720, 24.0f);

  EXPECT_EQ(Select(24.0f), smaller);

  m_options.matchResolution = true;
  EXPECT_EQ(Select(24.0f), smaller);
}
