/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/geometry/LiveGeometrySelector.h"

#include <optional>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

constexpr int WIDTH = 3840;
constexpr int HEIGHT = 2160;
const CRectInt CODED{0, 0, WIDTH, HEIGHT};

// The rectangles the vocabulary's entries occupy in this frame. What is served is always one
// of these, never a reading itself.
const CRectInt SCOPE{0, 280, WIDTH, HEIGHT - 280}; //!< 3840x1600, exactly 2.40
const CRectInt PILLAR{480, 0, WIDTH - 480, HEIGHT}; //!< 2880x2160, exactly 4:3

// What a frame of the scope body actually reads as - a line out from the entry's rectangle.
const CRectInt SCOPE_READ{0, 280, WIDTH, HEIGHT - 279};

// 2640x2160 is 1.22, which is within tolerance of no entry at all.
const CRectInt IMPLAUSIBLE{600, 0, WIDTH - 600, HEIGHT};

//! \brief A scope room, so a reading between 2.35 and 2.40 resolves the way the scan's would.
constexpr float AT_REST = 2.40f;

DetectionResult Reading(const CRectInt& rect)
{
  DetectionResult result;
  result.rect = rect;
  result.confidence = 0.8f;
  return result;
}

//! \brief A reading the detector produced but has no confidence in - a title card is text on
//! black, and its bounding box is whatever shape the words happen to make.
DetectionResult Unconfident(const CRectInt& rect)
{
  DetectionResult result;
  result.rect = rect;
  result.confidence = 0.0f;
  return result;
}

DetectionResult Dark()
{
  DetectionResult result;
  result.rect = CODED;
  result.degenerate = true;
  return result;
}

class TestLiveGeometrySelector : public ::testing::Test
{
protected:
  TestLiveGeometrySelector() { m_selector.Configure(CODED, 1.0f, AT_REST); }

  //! \brief Feed \p rect once, expecting nothing to change.
  void Quiet(const CRectInt& rect)
  {
    EXPECT_FALSE(m_selector.Feed(Reading(rect)).has_value()) << "unexpected change";
  }

  //! \brief Feed \p rect once, expecting it to be served.
  LiveGeometryReading Served(const CRectInt& rect)
  {
    const auto out = m_selector.Feed(Reading(rect));
    EXPECT_TRUE(out.has_value()) << "expected a change to serve";
    return out.value_or(LiveGeometryReading{});
  }

  CLiveGeometrySelector m_selector;
};

TEST_F(TestLiveGeometrySelector, FirstReadingOfAStreamIsServedAtOnce)
{
  // Nothing is established, so there is nothing to weigh the opening frame against.
  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect);
}

TEST_F(TestLiveGeometrySelector, AReadingIsServedAsTheRatioItResolvesTo)
{
  // Not the reading. Serving each frame's own rectangle would move the picture by a line
  // whenever the boundary rings, and the answer is a ratio rather than a measurement.
  const LiveGeometryReading out = Served(SCOPE_READ);
  EXPECT_EQ(SCOPE, out.rect);
  EXPECT_NE(SCOPE_READ, out.rect);
}

TEST_F(TestLiveGeometrySelector, ARatioTheMeasurementNeverFoundIsServedAnyway)
{
  // The whole point of reading every frame. A stored measurement samples a few dozen points
  // across a title, so a ratio used for one scene is one it was never likely to catch - and
  // refusing to serve it would make the one part of the system that can see it the one part
  // forbidden to act on it.
  Served(CODED);
  Quiet(PILLAR);
  Quiet(PILLAR);
  EXPECT_EQ(PILLAR, Served(PILLAR).rect);
}

TEST_F(TestLiveGeometrySelector, EveryRatioServedIsRemembered)
{
  // What a playback learned that the stored record does not have.
  Served(SCOPE_READ);
  Served(CODED);
  Quiet(PILLAR);
  Quiet(PILLAR);
  Served(PILLAR);

  ASSERT_EQ(3u, m_selector.Shapes().size());
  EXPECT_EQ(SCOPE, m_selector.Shapes()[0]);
  EXPECT_EQ(CODED, m_selector.Shapes()[1]);
  EXPECT_EQ(PILLAR, m_selector.Shapes()[2]);
}

TEST_F(TestLiveGeometrySelector, CutToAWiderRatioIsServedOnTheNextFrame)
{
  Served(SCOPE_READ);
  EXPECT_EQ(CODED, Served(CODED).rect);
}

TEST_F(TestLiveGeometrySelector, CutToANarrowerRatioIsServedAfterTheFrameGuard)
{
  Served(CODED);

  // Two frames of a shot that is dark top and bottom must not crop the picture...
  Quiet(SCOPE_READ);
  Quiet(SCOPE_READ);
  // ...the third says it is really a scope section.
  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect);
}

TEST_F(TestLiveGeometrySelector, TheFrameGuardCanBeTurnedOff)
{
  m_selector.SetParams({8, 1});
  Served(CODED);
  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect);
}

TEST_F(TestLiveGeometrySelector, ASingleOddFrameNeverMovesTheShape)
{
  Served(CODED);
  Quiet(SCOPE_READ);
  Quiet(CODED); // back to what is served: the run is abandoned
  Quiet(SCOPE_READ);
  Quiet(SCOPE_READ);
  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect);
}

TEST_F(TestLiveGeometrySelector, SeekServesWhereItLandedRatherThanWhereItLeft)
{
  Served(SCOPE_READ);
  m_selector.ResetForSeek();
  EXPECT_EQ(CODED, Served(CODED).rect);
}

TEST_F(TestLiveGeometrySelector, SeekIntoTheSameRatioServesNothing)
{
  Served(SCOPE_READ);
  m_selector.ResetForSeek();
  Quiet(SCOPE_READ);
}

TEST_F(TestLiveGeometrySelector, SeekIntoANarrowerRatioSkipsTheFrameGuard)
{
  Served(CODED);
  m_selector.ResetForSeek();
  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect);
}

TEST_F(TestLiveGeometrySelector, AWanderingBoundaryPublishesOnce)
{
  // Readings taken frame by frame from a dark scope passage at 4K. Each differs from the last
  // by more than the scan's eight-pixel tolerance, so matching on the rectangle published eight
  // shapes in half a second and stalled video output. They are all 2.40, and resolving to the
  // ratio rather than to the reading is what collapses them.
  const int wander[][2] = {{282, 1582}, {288, 1588}, {288, 1578}, {282, 1595},
                           {288, 1576}, {288, 1589}, {288, 1569}, {284, 1596}};

  Served(SCOPE_READ);
  for (const auto& reading : wander)
  {
    const CRectInt rect{0, reading[0], WIDTH, reading[0] + reading[1]};
    EXPECT_FALSE(m_selector.Feed(Reading(rect)).has_value())
        << "wander at y" << reading[0] << " height " << reading[1] << " published a change";
  }

  EXPECT_EQ(1u, m_selector.Shapes().size()) << "one ratio was seen, not nine";
}

TEST_F(TestLiveGeometrySelector, AShapeCorrespondingToNoRealRatioIsRefused)
{
  // Still a detector failure rather than a discovery: the vocabulary is what separates the two,
  // and it is the only thing that does now.
  Served(CODED);
  for (int i = 0; i < 10; ++i)
    Quiet(IMPLAUSIBLE);

  EXPECT_EQ(1u, m_selector.Shapes().size());
}

/*!
 * Measured on a scope episode's opening: the detector read the bounding box of the credits
 * text, honestly, at zero confidence. One of those boxes resolved to a real ratio and was
 * served over the actual picture for the rest of the episode.
 */
TEST_F(TestLiveGeometrySelector, AReadingTheDetectorHasNoConfidenceInIsRefused)
{
  Served(SCOPE_READ);

  // 3840x1746 is 2.20, a ratio that exists - the vocabulary cannot tell this from a real one.
  for (int i = 0; i < 10; ++i)
    EXPECT_FALSE(m_selector.Feed(Unconfident(CRectInt{0, 207, WIDTH, HEIGHT - 207})).has_value());

  ASSERT_EQ(1u, m_selector.Shapes().size()) << "a shape with no evidence behind it was recorded";
  EXPECT_EQ(SCOPE, *m_selector.Published());
}

TEST_F(TestLiveGeometrySelector, AnUnconfidentReadingNeitherConfirmsNorInterruptsARun)
{
  Served(CODED);
  Quiet(SCOPE_READ);
  EXPECT_FALSE(m_selector.Feed(Unconfident(SCOPE_READ)).has_value());
  Quiet(SCOPE_READ);
  // It counted for nothing and cost nothing: the third confident frame still commits.
  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect);
}

/*!
 * The gap between neighbouring ratios is smaller than a tolerance sized to absorb a wandering
 * boundary, which is why the comparison is against resolved rectangles and the floor stays
 * tight. 2.20 and 2.40 are seventy-three lines apart on this frame.
 */
TEST_F(TestLiveGeometrySelector, ANeighbouringRatioIsNotTheSameShape)
{
  const CRectInt TWENTY_TWO{0, 207, WIDTH, HEIGHT - 207};

  Served(TWENTY_TWO);
  Quiet(SCOPE_READ);
  Quiet(SCOPE_READ);
  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect) << "2.40 could not replace 2.20";
}

/*!
 * Exactly at the floor is the same edge, not a different one - the comparison is <=. No pair of
 * neighbouring entries lands on the default eight, so the boundary is reached by moving the
 * floor onto a pair rather than by finding a pair that sits on it: 2.20 and 2.40 are
 * seventy-three lines apart here, so seventy-three merges them and seventy-two does not.
 */
TEST_F(TestLiveGeometrySelector, ARatioExactlyTheFloorAwayIsTheSameShape)
{
  const CRectInt TWENTY_TWO{0, 207, WIDTH, HEIGHT - 207};

  m_selector.SetParams({73, 1});
  Served(TWENTY_TWO);
  Quiet(SCOPE_READ);

  EXPECT_EQ(1u, m_selector.Shapes().size()) << "the two were not read as one shape";
}

TEST_F(TestLiveGeometrySelector, ARatioOneLineFurtherIsADifferentShape)
{
  const CRectInt TWENTY_TWO{0, 207, WIDTH, HEIGHT - 207};

  m_selector.SetParams({72, 1});
  Served(TWENTY_TWO);

  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect);
  EXPECT_EQ(2u, m_selector.Shapes().size());
}

TEST_F(TestLiveGeometrySelector, DarkFramesChangeNothingAndInterruptNothing)
{
  Served(CODED);
  Quiet(SCOPE_READ);
  EXPECT_FALSE(m_selector.Feed(Dark()).has_value());
  Quiet(SCOPE_READ);
  // The dark frame neither counted towards the guard nor reset it.
  EXPECT_EQ(SCOPE, Served(SCOPE_READ).rect);
}

TEST_F(TestLiveGeometrySelector, ReadingTheServedShapeAgainServesNothing)
{
  Served(SCOPE_READ);
  Quiet(SCOPE_READ);
  Quiet(SCOPE);
}

TEST_F(TestLiveGeometrySelector, VariesOnlyOnceASecondShapeHasBeenServed)
{
  EXPECT_FALSE(Served(SCOPE_READ).varies);
  EXPECT_TRUE(Served(CODED).varies);
}

TEST_F(TestLiveGeometrySelector, TheRatioIsJudgedInDisplaySpace)
{
  // A 720x576 DVD displayed at 16:9 is coded at 1.25, which matches no entry; with the pixel
  // aspect applied it is 1.78 and resolves. The rectangle served is in coded pixels, so the
  // entry's ratio has to come back through the pixel aspect to land on the whole frame.
  CLiveGeometrySelector dvd;
  const CRectInt frame{0, 0, 720, 576};
  dvd.Configure(frame, (16.0f / 9.0f) / (720.0f / 576.0f));

  const auto out = dvd.Feed(Reading(frame));
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(frame, out->rect);
}

TEST_F(TestLiveGeometrySelector, ConfiguringForAStreamForgetsEverything)
{
  Served(SCOPE_READ);
  m_selector.Configure(CODED, 1.0f, AT_REST);

  EXPECT_FALSE(m_selector.HasPublished());
  EXPECT_TRUE(m_selector.Shapes().empty());
  EXPECT_FALSE(Served(SCOPE_READ).varies);
}

TEST_F(TestLiveGeometrySelector, UnconfiguredFeedsNothing)
{
  CLiveGeometrySelector fresh;
  EXPECT_FALSE(fresh.Feed(Reading(SCOPE)).has_value());
  EXPECT_FALSE(fresh.Feed(Reading(CODED)).has_value());
}

} // namespace
