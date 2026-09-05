/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/AspectRatioVocabulary.h"
#include "video/geometry/EffectiveGeometry.h"
#include "video/geometry/GeometryPublication.h"
#include "video/geometry/test/GeometryTestHelpers.h"

#include <tuple>
#include <utility>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;
using namespace KODI::VIDEO::GEOMETRY::TEST;
using namespace KODI::UTILS;

TEST(TestEffectiveGeometry, AnamorphicContentResolvesToItsDisplayedRatio)
{
  const GeometryInputs inputs = ScopeCachedPal();

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  // Measured at 2.3486 in display pixels, which resolves to the 2.35 entry - the answer is
  // the ratio the content was shot at, not the pixels of this encode.
  EXPECT_EQ(GeometrySource::Cached, result.source);
  EXPECT_FLOAT_EQ(2.35f, result.aspect);
  EXPECT_EQ("2.35", result.label);
  ExpectRect(result.displayRect, 0.0f, 70.13f, 1024.0f, 505.87f);
}

TEST(TestEffectiveGeometry, AMeasurementResolvesToItsVocabularyEntry)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  // 3840x1728 is 2.2222, an encode of a 2.20 film. The answer is the entry, not the encode:
  // a masking system absorbs the difference in the fabric, and what it cannot absorb is a
  // ratio nothing was ever shot at, which no preset can be programmed against.
  inputs.cached = Cached(CRectInt{0, 216, 3840, 1944}, CRectInt{0, 216, 3840, 1944});

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_FLOAT_EQ(2.20f, result.aspect);
  EXPECT_EQ("2.20", result.label);
  EXPECT_EQ("Todd-AO", result.name);
  ExpectRect(result.displayRect, 0.0f, 207.27f, 3840.0f, 1952.73f);
}

TEST(TestEffectiveGeometry, SnappingErrsTowardTheShapeTheRoomRestsAt)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  // 3840x1620 is 2.3704, within tolerance of 2.35 and 2.40 both. Which entry is right
  // depends on the room, so the resting shape decides - in both directions.
  inputs.cached = Cached(CRectInt{0, 270, 3840, 1890}, CRectInt{0, 270, 3840, 1890});

  inputs.atRestAspect = 1.78f;
  const EffectiveGeometry towardFlat = ResolveEffectiveGeometry(inputs);
  EXPECT_EQ("2.35", towardFlat.label);

  inputs.atRestAspect = 2.40f;
  const EffectiveGeometry towardScope = ResolveEffectiveGeometry(inputs);
  EXPECT_EQ("2.40", towardScope.label);
  ExpectRect(towardScope.displayRect, 0.0f, 280.0f, 3840.0f, 1880.0f);
}

TEST(TestEffectiveGeometry, DetectionAndDeclarationOfTheSameRatioAgreeExactly)
{
  GeometryInputs measured;
  measured.stream = Uhd();
  measured.cached = Cached(CRectInt{0, 270, 3840, 1890}, CRectInt{0, 270, 3840, 1890});
  measured.atRestAspect = 2.40f;

  GeometryInputs declared;
  declared.stream = Uhd();
  declared.declaredAspect = CAspectRatioVocabulary::RatioForKey(240);

  const EffectiveGeometry fromMeasurement = ResolveEffectiveGeometry(measured);
  const EffectiveGeometry fromDeclaration = ResolveEffectiveGeometry(declared);

  // Identical, not merely close: both took FitAspect() of the same entry over the same
  // frame, so nothing downstream can tell how the answer was arrived at.
  EXPECT_EQ(fromDeclaration.displayRect.x1, fromMeasurement.displayRect.x1);
  EXPECT_EQ(fromDeclaration.displayRect.y1, fromMeasurement.displayRect.y1);
  EXPECT_EQ(fromDeclaration.displayRect.x2, fromMeasurement.displayRect.x2);
  EXPECT_EQ(fromDeclaration.displayRect.y2, fromMeasurement.displayRect.y2);
  EXPECT_EQ(fromDeclaration.aspect, fromMeasurement.aspect);
  EXPECT_EQ(fromDeclaration.label, fromMeasurement.label);
}

/*!
 * What the measurement alone gave, which is recorded alongside a declaration so a corrected
 * detection can be told from one that never had an answer.
 *
 * It is taken during the resolution rather than by resolving a second time with the
 * declaration blanked, so the two have to be shown to agree - and the declaration must not
 * reach it.
 */
TEST(TestEffectiveGeometry, TheDetectedRatioIgnoresADeclarationOverIt)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 216, 3840, 1944}, CRectInt{0, 216, 3840, 1944});

  // Undeclared, the served answer and the detected one are the same thing.
  const EffectiveGeometry undeclared = ResolveEffectiveGeometry(inputs);
  EXPECT_FLOAT_EQ(2.20f, undeclared.aspect);
  EXPECT_FLOAT_EQ(2.20f, undeclared.detectedAspect);
  EXPECT_FLOAT_EQ(2.20f, ResolveDetectedAspect(inputs));

  // Declared over, the served answer moves and the detected one does not.
  inputs.declaredAspect = CAspectRatioVocabulary::RatioForKey(240);
  const EffectiveGeometry declared = ResolveEffectiveGeometry(inputs);
  EXPECT_EQ(GeometrySource::Declared, declared.source);
  EXPECT_FLOAT_EQ(2.40f, declared.aspect);
  EXPECT_FLOAT_EQ(2.20f, declared.detectedAspect);
  EXPECT_FLOAT_EQ(2.20f, ResolveDetectedAspect(inputs));
}

//! Nothing measured is zero rather than the frame's own ratio, so a declaration made over a
//! title that was never measured is not recorded as having corrected anything.
TEST(TestEffectiveGeometry, TheDetectedRatioIsZeroWhenNothingWasMeasured)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  EXPECT_EQ(GeometrySource::Container, ResolveEffectiveGeometry(inputs).source);
  EXPECT_FLOAT_EQ(0.0f, ResolveDetectedAspect(inputs));

  inputs.declaredAspect = CAspectRatioVocabulary::RatioForKey(240);
  EXPECT_FLOAT_EQ(0.0f, ResolveDetectedAspect(inputs));

  // A measurement the plausibility gate refused is not a measurement either. 3840x1000 is
  // 3.84, which no real ratio is.
  GeometryInputs rejected;
  rejected.stream = Uhd();
  rejected.cached = Cached(CRectInt{0, 580, 3840, 1580}, CRectInt{0, 580, 3840, 1580});
  const EffectiveGeometry result = ResolveEffectiveGeometry(rejected);
  ASSERT_TRUE(result.rejected);
  EXPECT_FLOAT_EQ(0.0f, result.detectedAspect);
}

TEST(TestEffectiveGeometry, AFullFrameMeasurementServesExactlyTheFrame)
{
  // The common case in any library: uncropped 16:9. The entry is the exact fraction, so
  // resolving to it gives back the frame to the pixel and nothing on the wire changes.
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 0, 3840, 2160}, CRectInt{0, 0, 3840, 2160});

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Cached, result.source);
  EXPECT_EQ(0.0f, result.displayRect.x1);
  EXPECT_EQ(0.0f, result.displayRect.y1);
  EXPECT_EQ(3840.0f, result.displayRect.x2);
  EXPECT_EQ(2160.0f, result.displayRect.y2);
}

TEST(TestEffectiveGeometry, SnappingCentresWhatItServes)
{
  // The measured offset is discarded with the measured size, deliberately: the answer is the
  // entry's rectangle, which is a declaration's, and a declaration knows only a ratio.
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 100, 3840, 1700}, CRectInt{0, 100, 3840, 1700});

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ("2.40", result.label);
  ExpectRect(result.displayRect, 0.0f, 280.0f, 3840.0f, 1880.0f);
}

TEST(TestEffectiveGeometry, NothingKnownReportsTheWholeFrame)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Container, result.source);
  ExpectRect(result.displayRect, 0.0f, 0.0f, 3840.0f, 2160.0f);
  EXPECT_EQ("1.78", result.label);
}

TEST(TestEffectiveGeometry, AMeasurementThatIsOnNoRealRatioIsRefused)
{
  const GeometryInputs inputs = RefusedCachedUhd();

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Container, result.source);
  ExpectRect(result.displayRect, 0.0f, 0.0f, 3840.0f, 2160.0f);
}

//! A deliberate exception to never-narrower, pinned so it stays deliberate: a superseded
//! detector's answer is still served rather than discarded, because it is the best available
//! and discarding it would report the whole frame as picture on every title ever measured.
TEST(TestEffectiveGeometry, AStaleMeasurementIsStillServed)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 280, 3840, 1880}, false,
                         ContentGeometryState::STALE);

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Cached, result.source);
  EXPECT_TRUE(result.stale);
  EXPECT_EQ("2.40", result.label);
  ExpectRect(result.displayRect, 0.0f, 280.0f, 3840.0f, 1880.0f);
}

TEST(TestEffectiveGeometry, RefusedMeasurementIsNotReportedStale)
{
  GeometryInputs inputs = RefusedCachedUhd();
  inputs.cached.state = ContentGeometryState::STALE;

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Container, result.source);
  EXPECT_FALSE(result.stale);
}

TEST(TestEffectiveGeometry, ADeclarationOutranksAMeasurement)
{
  GeometryInputs inputs = ScopeCachedUhd();
  inputs.declaredAspect = 1.85f;

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Declared, result.source);
  EXPECT_EQ("1.85", result.label);
}

TEST(TestEffectiveGeometry, ADeclarationIsNotPlausibilityChecked)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  // Nothing measured could survive at this ratio; a person saying it stands regardless.
  inputs.declaredAspect = 2.76f;

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Declared, result.source);
  EXPECT_NEAR(2.76f, result.aspect, 0.001f);
}

TEST(TestEffectiveGeometry, ADeclarationIsCentredInTheFrame)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.declaredAspect = 2.40f;

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  ExpectRect(result.displayRect, 0.0f, 280.0f, 3840.0f, 1880.0f);
}

TEST(TestEffectiveGeometry, ADeclarationOfTheFramesOwnRatioIsTheFrame)
{
  GeometryInputs inputs;
  inputs.stream = StreamGeometry{CRectInt{0, 0, 1920, 800}, 2.4f, 0};
  inputs.declaredAspect = 2.40f;

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  // Exactly the frame, not a fraction of a pixel inside it.
  EXPECT_EQ(0.0f, result.displayRect.x1);
  EXPECT_EQ(0.0f, result.displayRect.y1);
  EXPECT_EQ(1920.0f, result.displayRect.x2);
  EXPECT_EQ(800.0f, result.displayRect.y2);
}

TEST(TestEffectiveGeometry, ADeclarationNarrowerThanTheFrameInsetsTheSides)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.declaredAspect = 1.33f;

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  ExpectRect(result.displayRect, 483.6f, 0.0f, 3356.4f, 2160.0f);
}

TEST(TestEffectiveGeometry, ALiveReadingOutranksTheCache)
{
  GeometryInputs inputs = ScopeCachedUhd();
  inputs.live.hasReading = true;
  inputs.live.rect = CRectInt{0, 0, 3840, 2160};
  inputs.live.envelope = CRectInt{0, 0, 3840, 2160};
  inputs.hasLive = true;

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Live, result.source);
  EXPECT_EQ("1.78", result.label);
}

// A title that is mostly scope with full-frame sequences in it. This is the only shape where
// the two policies disagree, and both answers are correct for the consumer that asked.
TEST(TestEffectiveGeometry, VariablePolicyDecidesBetweenShowingAllOfItAndNamingItsRatio)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 3840, 2160}, true);

  inputs.policy = VariableGeometryPolicy::Envelope;
  const EffectiveGeometry envelope = ResolveEffectiveGeometry(inputs);
  EXPECT_EQ("1.78", envelope.label);
  ExpectRect(envelope.displayRect, 0.0f, 0.0f, 3840.0f, 2160.0f);
  EXPECT_TRUE(envelope.varies);

  inputs.policy = VariableGeometryPolicy::Dominant;
  const EffectiveGeometry dominant = ResolveEffectiveGeometry(inputs);
  EXPECT_EQ("2.40", dominant.label);
  ExpectRect(dominant.displayRect, 0.0f, 280.0f, 3840.0f, 1880.0f);
  EXPECT_TRUE(dominant.varies);
}

// A scope title carrying a pillarboxed archive sequence. The variation is horizontal, so the
// taller of the two rectangles is also the narrower one, and taking the outer extent per axis
// is what keeps the answer at the ratio the film is in.
TEST(TestEffectiveGeometry, HorizontalVariationDoesNotNarrowTheAnswer)
{
  const StreamGeometry scope{CRectInt{0, 0, 1920, 800}, 2.4f, 0};
  const CRectInt body{0, 0, 1920, 800};
  const CRectInt archive{413, 0, 1507, 800};

  CombinedGeometry live;
  live.hasReading = true;
  live.rect = body;
  live.envelope = body; // the union of body and archive is body
  live.varies = false;

  GeometryInputs inputs;
  inputs.stream = scope;
  inputs.hasLive = true;
  inputs.live = live;
  inputs.sections = {body, archive};

  for (const VariableGeometryPolicy policy :
       {VariableGeometryPolicy::Envelope, VariableGeometryPolicy::Dominant})
  {
    inputs.policy = policy;
    const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);
    EXPECT_EQ("2.40", result.label);
    ExpectRect(result.displayRect, 0.0f, 0.0f, 1920.0f, 800.0f);
  }
}

/*!
 * A section is published in display space and named the same way the served rectangle is, so
 * a consumer does not have to reimplement Kodi's bucketing to describe one. Anamorphic,
 * because that is where a section divided in its own coded pixels gives a different answer
 * from the published one.
 */
TEST(TestEffectiveGeometry, MeasuredSectionsArePublishedInDisplaySpaceWithTheirOwnRatio)
{
  GeometryInputs inputs = ScopeCachedPal(CRectInt{0, 0, 720, 576}, true);
  inputs.sections = {CRectInt{0, 70, 720, 506}, CRectInt{0, 0, 720, 576}};

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  // The scope section resolves to its 2.35 entry; the full-frame section is exactly 16:9 and
  // resolves back to exactly the frame.
  ASSERT_EQ(2u, result.sections.size());
  ExpectRect(result.sections[0].displayRect, 0.0f, 70.13f, 1024.0f, 505.87f);
  ExpectRect(result.sections[1].displayRect, 0.0f, 0.0f, 1024.0f, 576.0f);

  // Each section's ratio is its entry's, exactly - the same resolution the served rectangle
  // gets, so choosing a section over it is choosing between like and like.
  EXPECT_FLOAT_EQ(2.35f, result.sections[0].aspect);
  EXPECT_EQ("2.35", result.sections[0].label);
  EXPECT_FLOAT_EQ(16.0f / 9.0f, result.sections[1].aspect);
  EXPECT_EQ("1.78", result.sections[1].label);
}

/*!
 * Half side-by-side: each view is 960x1080 of a 1920x1080 frame, and its pixels are squeezed
 * two to one, so a 2.39 picture in it measures 960 by 816. Resolved against the view that is
 * 2.35; resolved against the whole frame it is 1.18, which is inside tolerance of Movietone -
 * a real entry, so nothing rejects it and the title is published as 1.19.
 */
TEST(TestEffectiveGeometry, AStereoscopicMeasurementResolvesAgainstItsOwnView)
{
  const CRectInt content{0, 132, 960, 948};

  GeometryInputs view;
  view.stream = MeasuredStreamGeometry("left_right", 1920, 1080, 16.0f / 9.0f);
  view.cached = Cached(content, content);
  EXPECT_EQ("2.35", ResolveEffectiveGeometry(view).label);

  // Against the whole frame the same measurement names some other ratio. Which one is not the
  // point and is not asserted - it depends on where the vocabulary's entries happen to sit, and
  // pinning it would make an unrelated edit to the vocabulary fail here.
  GeometryInputs wholeFrame;
  wholeFrame.stream = {CRectInt{0, 0, 1920, 1080}, 16.0f / 9.0f, 0};
  wholeFrame.cached = Cached(content, content);
  EXPECT_NE("2.35", ResolveEffectiveGeometry(wholeFrame).label)
      << "the region the measurement describes has stopped mattering, which it should not have";
}

/*!
 * A measurement describes the stream the sampler took, so it is withheld while another one is
 * decoding - withheld, not destroyed. The inputs are loaded once, as the file opens, so
 * clearing them would leave a viewer who switched away and back watching the coded frame for
 * the rest of the playback, reported as container exactly as an unmeasured title is.
 */
TEST(TestEffectiveGeometry, AnotherStreamWithholdsTheRecordWithoutDestroyingIt)
{
  GeometryInputs inputs = ScopeCachedUhd();
  inputs.sections = {CRectInt{0, 280, 3840, 1880}};

  const GeometryInputs other = InputsForStream(inputs, 1);
  EXPECT_FALSE(other.cached.HasRecord());
  EXPECT_TRUE(other.sections.empty());
  EXPECT_EQ(GeometrySource::Container, ResolveEffectiveGeometry(other).source);

  EXPECT_TRUE(inputs.cached.HasRecord()) << "the stored inputs were mutated";

  const GeometryInputs back = InputsForStream(inputs, 0);
  EXPECT_EQ(GeometrySource::Cached, ResolveEffectiveGeometry(back).source);
  EXPECT_EQ(1u, back.sections.size());
}

//! A live reading is taken from whatever is decoding now, so it describes the stream in force
//! whichever that is and is not withheld with the record.
TEST(TestEffectiveGeometry, AnotherStreamKeepsTheLiveReading)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.hasLive = true;
  inputs.live.hasReading = true;
  inputs.live.rect = CRectInt{0, 280, 3840, 1880};
  inputs.live.envelope = inputs.live.rect;

  EXPECT_EQ(GeometrySource::Live, ResolveEffectiveGeometry(InputsForStream(inputs, 1)).source);
}

/*!
 * The schema says a label is empty when the ratio corresponds to none, because a client
 * compares it. Nearest() never rejects, so a section on no real ratio was published under the
 * nearest entry's label - indistinguishable from a title actually shot at it.
 */
TEST(TestEffectiveGeometry, ASectionOnNoRealRatioCarriesNoLabel)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 3840, 2160}, true);
  inputs.sections = {CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 1080, 2160}};

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  ASSERT_EQ(2u, result.sections.size());
  EXPECT_EQ("2.40", result.sections[0].label);
  EXPECT_TRUE(result.sections[1].label.empty()) << "published as " << result.sections[1].label;
  EXPECT_TRUE(result.sections[1].name.empty());
}

/*!
 * The same contract on the rectangle itself, which is the one Player.GetProperties and every
 * skin label read. The frame here is on no entry at all, so the answer served for it has no
 * name - and reporting the nearest would say a shape nothing was shot at is a real ratio.
 */
TEST(TestEffectiveGeometry, ARectangleOnNoRealRatioCarriesNoLabel)
{
  GeometryInputs inputs;
  inputs.stream = {CRectInt{0, 0, 1080, 2160}, 0.5f, 0};

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_NEAR(0.5f, result.aspect, 0.01f);
  EXPECT_TRUE(result.label.empty()) << "published as " << result.label;
  EXPECT_TRUE(result.name.empty());
}

//! Never narrower under uncertainty, on the path where the file itself has changed. The row
//! describes a different encode, so its rectangle may be narrower than this file's picture -
//! and a rectangle that is too narrow closes a mask over real picture.
TEST(TestEffectiveGeometry, ARecordForADifferentFileIsNotServed)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  //! What CVideoDatabase::GetContentGeometry returns once the identity check fails.
  inputs.cached = ContentGeometryLookup{};

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Container, result.source);
  ExpectRect(result.displayRect, 0.0f, 0.0f, 3840.0f, 2160.0f);
}

//! A scan that opened the file and read nothing still stores a row, so that the sweep does not
//! attempt it again every run. That row must not be mistaken for a measurement.
TEST(TestEffectiveGeometry, ARecordThatCarriesNoReadingFallsBackToTheFrame)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached.state = ContentGeometryState::VALID;
  inputs.cached.record.hasReading = false;
  inputs.cached.record.rect = CRectInt{0, 280, 3840, 1880};
  inputs.cached.record.envelope = CRectInt{0, 280, 3840, 1880};

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_EQ(GeometrySource::Container, result.source);
  EXPECT_FALSE(result.rejected) << "nothing was measured, so nothing was rejected";
  ExpectRect(result.displayRect, 0.0f, 0.0f, 3840.0f, 2160.0f);
}

//! A rejected measurement and no measurement at all both serve the frame, so without this
//! flag they are indistinguishable - and the right response to each is the opposite one.
TEST(TestEffectiveGeometry, AGateRejectionIsReportedRatherThanJustActedOn)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  //! 3456x2160 is 1.60, which is 3.7% from the nearest detectable entry - beyond tolerance,
  //! so the gate rejects it and the coded frame stands.
  inputs.cached = Cached(CRectInt{192, 0, 3648, 2160}, CRectInt{192, 0, 3648, 2160});

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_TRUE(result.rejected);
  EXPECT_EQ(GeometrySource::Container, result.source);
  //! Never narrower under uncertainty: the frame, not the rejected rectangle.
  ExpectRect(result.displayRect, 0.0f, 0.0f, 3840.0f, 2160.0f);
}

TEST(TestEffectiveGeometry, AnAcceptedMeasurementIsNotReportedAsRejected)
{
  GeometryInputs inputs = ScopeCachedUhd();

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_FALSE(result.rejected);
  EXPECT_EQ(GeometrySource::Cached, result.source);
}

//! Pinning a title whose measurement came out wrong is what declaring is for, and a measurement
//! wrong enough to be refused outright is the case it is most needed on. The flag says a
//! measurement is being served in place of, so it cannot outlive being served over.
TEST(TestEffectiveGeometry, ADeclarationClearsAGateRejection)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{192, 0, 3648, 2160}, CRectInt{192, 0, 3648, 2160});

  ASSERT_TRUE(ResolveEffectiveGeometry(inputs).rejected) << "the gate has to refuse it first";

  inputs.declaredAspect = 2.40f;

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_FALSE(result.rejected);
  EXPECT_EQ(GeometrySource::Declared, result.source);
  ExpectRect(result.displayRect, 0.0f, 280.0f, 3840.0f, 1880.0f);
}

TEST(TestEffectiveGeometry, NothingMeasuredIsNotAGateRejection)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  EXPECT_FALSE(result.rejected) << "no measurement was taken, so none was rejected";
  EXPECT_EQ(GeometrySource::Container, result.source);
}

//! A quarter turn is applied to a section exactly as it is to the answer, and its ratio stays
//! the upright one - the vocabulary has no name below 1.00 for the turned figure to be given.
TEST(TestEffectiveGeometry, AQuarterTurnLeavesASectionsRatioUpright)
{
  GeometryInputs inputs;
  inputs.stream = Uhd(90);
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 3840, 2160}, true);
  inputs.sections = {CRectInt{0, 280, 3840, 1880}};

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  ASSERT_EQ(1u, result.sections.size());
  ExpectRect(result.sections[0].displayRect, 280.0f, 0.0f, 1880.0f, 3840.0f);
  EXPECT_NEAR(2.40f, result.sections[0].aspect, 0.01f);
  EXPECT_EQ("2.40", result.sections[0].label);
}

//! The distinction the count exists for: a title detected as 1.78 and a title never detected
//! at all serve the same rectangle, and a consumer showing a badge has to tell them apart.
TEST(TestEffectiveGeometry, AnUnmeasuredTitleContainsNoRatios)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  const ContentAspectSet set = ContentAspectsOf(ResolveEffectiveGeometry(inputs));

  EXPECT_TRUE(set.aspects.empty());
  EXPECT_EQ(GeometrySource::Container, set.source);
}

//! A rejected measurement serves the frame as well, and the frame's own ratio is no more the
//! content's here than it is above.
TEST(TestEffectiveGeometry, ARefusedMeasurementContainsNoRatiosEither)
{
  const GeometryInputs inputs = RefusedCachedUhd();

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  ASSERT_TRUE(result.rejected);
  EXPECT_TRUE(ContentAspectsOf(result).aspects.empty());
}

//! Nothing retained the per-sample detail, so there are no sections - the ratio the resolver
//! settled on is the whole of what the title contains rather than nothing at all.
TEST(TestEffectiveGeometry, AMeasuredTitleWithNoRetainedDetailContainsTheRatioItResolvedTo)
{
  GeometryInputs inputs = ScopeCachedUhd();

  const ContentAspectSet set = ContentAspectsOf(ResolveEffectiveGeometry(inputs));

  ASSERT_EQ(1u, set.aspects.size());
  EXPECT_EQ("2.40", set.aspects[0].label);
  EXPECT_EQ("Scope", set.aspects[0].name);
  EXPECT_EQ(GeometrySource::Cached, set.source);
  EXPECT_FALSE(set.varies);
}

TEST(TestEffectiveGeometry, ADeclaredTitleContainsTheRatioThatWasDeclared)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.declaredAspect = 1.85f;

  const ContentAspectSet set = ContentAspectsOf(ResolveEffectiveGeometry(inputs));

  ASSERT_EQ(1u, set.aspects.size());
  EXPECT_EQ("1.85", set.aspects[0].label);
  EXPECT_EQ(GeometrySource::Declared, set.source);
}

TEST(TestEffectiveGeometry, TheRatiosOfAVariableTitleArePublishedDominantFirst)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 3840, 2160}, true);
  inputs.sections = {CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 3840, 2160}};

  const ContentAspectSet set = ContentAspectsOf(ResolveEffectiveGeometry(inputs));

  ASSERT_EQ(2u, set.aspects.size());
  EXPECT_EQ("2.40", set.aspects[0].label);
  EXPECT_EQ("1.78", set.aspects[1].label);
  EXPECT_TRUE(set.varies);
}

//! The clusters are share-weighted, so the same ratio can be measured twice over. What is
//! published is the set of ratios the title contains, never how many stretches hold each.
TEST(TestEffectiveGeometry, TheSameRatioMeasuredTwiceIsStillOneRatio)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 279, 3840, 1881}, true);
  inputs.sections = {CRectInt{0, 280, 3840, 1880}, CRectInt{0, 281, 3840, 1879}};

  const ContentAspectSet set = ContentAspectsOf(ResolveEffectiveGeometry(inputs));

  ASSERT_EQ(1u, set.aspects.size());
  EXPECT_EQ("2.40", set.aspects[0].label);

  // Still varying: the measurement saw the picture move, and one rectangle covering both
  // stretches is a fact about the title that the ratio alone does not carry.
  EXPECT_TRUE(set.varies);
}

//! A title letterboxed in one stretch and pillarboxed in another is served at the frame that
//! covers both, and that shape is one the title is never in. What it contains is its own two
//! ratios, which is what a badge says and what a masking system programmes against.
TEST(TestEffectiveGeometry, TheShapeAVariableTitleIsServedAtIsNotOneOfItsRatios)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 3840, 2160}, true);
  inputs.sections = {CRectInt{0, 280, 3840, 1880}, CRectInt{480, 0, 3360, 2160}};

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  ASSERT_EQ("1.78", result.label);

  const ContentAspectSet set = ContentAspectsOf(result);
  ASSERT_EQ(2u, set.aspects.size());
  EXPECT_EQ("2.40", set.aspects[0].label);
  EXPECT_EQ("1.33", set.aspects[1].label);
}

//! Drawn() is what decides whether the payload carries a screen at all, and an empty rectangle
//! published as one is a rectangle a consumer would drive a mask to.
TEST(TestEffectiveGeometry, NothingIsDrawnUntilThereIsAPicture)
{
  EXPECT_FALSE(DrawnGeometry{}.Drawn());

  DrawnGeometry rasterOnly;
  rasterOnly.raster = CRect{0.0f, 0.0f, 1920.0f, 800.0f};
  EXPECT_FALSE(rasterOnly.Drawn());

  DrawnGeometry drawn;
  drawn.picture = CRect{0.0f, 0.0f, 1920.0f, 800.0f};
  EXPECT_TRUE(drawn.Drawn());
}

TEST(TestEffectiveGeometry, RotationDoesNotDisqualifyTheRatioItWasShotAt)
{
  GeometryInputs inputs;
  inputs.stream = Uhd(90);
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 280, 3840, 1880});

  const EffectiveGeometry result = ResolveEffectiveGeometry(inputs);

  // The measurement survives, and is reported turned.
  EXPECT_EQ(GeometrySource::Cached, result.source);
  EXPECT_EQ("2.40", result.label);
  EXPECT_EQ(90, result.orientation);
  ExpectRect(result.displayFrame, 0.0f, 0.0f, 2160.0f, 3840.0f);
  ExpectRect(result.displayRect, 280.0f, 0.0f, 1880.0f, 3840.0f);
}

TEST(TestEffectiveGeometry, UncroppedContentIsDrawnWhereTheWholeVideoIs)
{
  EffectiveGeometry geometry;
  geometry.displayFrame = {0.0f, 0.0f, 3840.0f, 2160.0f};
  geometry.displayRect = geometry.displayFrame;

  // Exactly the video rectangle, so a file with no measurement and no declaration cannot move
  // anything: the resolver reports the coded frame in that case.
  ExpectRect(PictureRect(geometry, CRect{0.0f, 0.0f, 1920.0f, 1080.0f}), 0.0f, 0.0f, 1920.0f,
             1080.0f);
}

TEST(TestEffectiveGeometry, TheBarsAreTakenAsAFractionOfTheFrame)
{
  EffectiveGeometry geometry;
  geometry.displayFrame = {0.0f, 0.0f, 3840.0f, 2160.0f};
  geometry.displayRect = {0.0f, 280.0f, 3840.0f, 1880.0f};

  // 280 of 2160 is 12.96%, which of a 1080 high video is 140.
  ExpectRect(PictureRect(geometry, CRect{0.0f, 0.0f, 1920.0f, 1080.0f}), 0.0f, 140.0f, 1920.0f,
             940.0f);
}

TEST(TestEffectiveGeometry, ThePictureFollowsAVideoDrawnSmallerThanTheScreen)
{
  EffectiveGeometry geometry;
  geometry.displayFrame = {0.0f, 0.0f, 3840.0f, 2160.0f};
  geometry.displayRect = {0.0f, 280.0f, 3840.0f, 1880.0f};

  // A 16:9 video on a 4:3 screen is drawn pillarboxed, and the picture is inside that.
  ExpectRect(PictureRect(geometry, CRect{0.0f, 180.0f, 1440.0f, 990.0f}), 0.0f, 285.0f, 1440.0f,
             885.0f);
}

TEST(TestEffectiveGeometry, AZoomedVideoCarriesThePictureOffTheScreenWithIt)
{
  EffectiveGeometry geometry;
  geometry.displayFrame = {0.0f, 0.0f, 3840.0f, 2160.0f};
  geometry.displayRect = {0.0f, 280.0f, 3840.0f, 1880.0f};

  // Zoomed by 2160/1600 to put the bars outside the raster, which is how an anamorphic lens
  // without masking is driven. The picture then lands exactly on the screen, and confining the
  // interface to it comes to nothing - the correct answer, reached without a case for it.
  ExpectRect(PictureRect(geometry, CRect{0.0f, -189.0f, 1920.0f, 1269.0f}), 0.0f, 0.0f, 1920.0f,
             1080.0f);
}

TEST(TestEffectiveGeometry, AnOffCentrePictureIsNotSymmetrised)
{
  EffectiveGeometry geometry;
  geometry.displayFrame = {0.0f, 0.0f, 1920.0f, 1080.0f};
  geometry.displayRect = {0.0f, 40.0f, 1920.0f, 1000.0f};

  ExpectRect(PictureRect(geometry, CRect{0.0f, 0.0f, 1920.0f, 1080.0f}), 0.0f, 40.0f, 1920.0f,
             1000.0f);
}

TEST(TestEffectiveGeometry, APillarboxedPictureInsetsTheSides)
{
  EffectiveGeometry geometry;
  geometry.displayFrame = {0.0f, 0.0f, 3840.0f, 2160.0f};
  geometry.displayRect = {480.0f, 0.0f, 3360.0f, 2160.0f};

  ExpectRect(PictureRect(geometry, CRect{0.0f, 0.0f, 1920.0f, 1080.0f}), 240.0f, 0.0f, 1680.0f,
             1080.0f);
}

TEST(TestEffectiveGeometry, AFrameOfNoSizeLeavesTheVideoRectangleAlone)
{
  EffectiveGeometry geometry;

  ExpectRect(PictureRect(geometry, CRect{0.0f, 0.0f, 1920.0f, 1080.0f}), 0.0f, 0.0f, 1920.0f,
             1080.0f);
}

TEST(TestEffectiveGeometry, ARotatedPictureIsMappedOntoTheRotatedVideoRectangle)
{
  GeometryInputs inputs;
  inputs.stream = Uhd(90);
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 280, 3840, 1880});

  // Turned, the 2.40 picture is pillarboxed within a portrait frame, and the video rectangle it
  // is drawn into is portrait too. No rotation is applied here - it is already in both.
  ExpectRect(PictureRect(ResolveEffectiveGeometry(inputs), CRect{660.0f, 0.0f, 1260.0f, 1080.0f}),
             737.78f, 0.0f, 1182.22f, 1080.0f);
}

TEST(TestEffectiveGeometry, TheSourceRegionOfAnUnmeasuredFrameIsTheFrame)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  const CRect frame{0.0f, 0.0f, 3840.0f, 2160.0f};
  ExpectRect(SourceRect(RenderGeometryOf(ResolveEffectiveGeometry(inputs)), frame), 0.0f, 0.0f,
             3840.0f, 2160.0f);

  // The resting shape carries no rectangle at all, and must leave the caller drawing what it
  // always drew rather than cutting the picture to a rectangle from nowhere.
  ExpectRect(SourceRect(RenderGeometryOf(EffectiveGeometry{}), frame), 0.0f, 0.0f, 3840.0f,
             2160.0f);
}

TEST(TestEffectiveGeometry, BakedBarsAreCutFromTheSourceRegion)
{
  GeometryInputs inputs = ScopeCachedUhd();

  ExpectRect(SourceRect(RenderGeometryOf(ResolveEffectiveGeometry(inputs)),
                        CRect{0.0f, 0.0f, 3840.0f, 2160.0f}),
             0.0f, 280.0f, 3840.0f, 1880.0f);
}

TEST(TestEffectiveGeometry, TheSourceRegionIsCutInCodedPixelsHoweverTheyAreShaped)
{
  // A scope picture on an anamorphic PAL DVD: the resolved rectangle lives in display space,
  // 1024 wide, and the region actually sampled is 720 coded pixels across. A fraction of the
  // width survives the pixel-aspect correction exactly, which is why no scale appears here.
  const GeometryInputs inputs = ScopeCachedPal();

  ExpectRect(SourceRect(RenderGeometryOf(ResolveEffectiveGeometry(inputs)),
                        CRect{0.0f, 0.0f, 720.0f, 576.0f}),
             0.0f, 70.13f, 720.0f, 505.87f);
}

TEST(TestEffectiveGeometry, TheSourceRegionOfOneStereoscopicViewKeepsItsFractions)
{
  GeometryInputs inputs = ScopeCachedUhd();

  // The caller hands in one view of a top-and-bottom frame, whose bars are the same fraction
  // of each view, and the cut lands within that view rather than within the whole frame.
  ExpectRect(SourceRect(RenderGeometryOf(ResolveEffectiveGeometry(inputs)),
                        CRect{0.0f, 0.0f, 3840.0f, 1080.0f}),
             0.0f, 140.0f, 3840.0f, 940.0f);
}

TEST(TestEffectiveGeometry, ATurnedPictureIsCutInCodedSpace)
{
  // Deliberately off-centre bars, because a symmetric rectangle comes back the same whichever
  // way the turn is undone - only asymmetry can catch the inverse being the turn itself.
  // Constructed rather than resolved: resolution centres what it serves, so an off-centre
  // rectangle only reaches SourceRect() from a consumer that built its own geometry.
  EffectiveGeometry geometry;
  geometry.codedFrame = CRectInt{0, 0, 3840, 2160};
  geometry.displayFrame = CRect{0.0f, 0.0f, 2160.0f, 3840.0f};
  geometry.displayRect = CRect{460.0f, 0.0f, 2060.0f, 3840.0f};
  geometry.orientation = 90;

  ExpectRect(SourceRect(RenderGeometryOf(geometry), CRect{0.0f, 0.0f, 3840.0f, 2160.0f}), 0.0f,
             100.0f, 3840.0f, 1700.0f);
}

TEST(TestEffectiveGeometry, TheSourceRegionIsNeverAnotherShapeThanTheContent)
{
  // The pin for "nothing is ever stretched": a renderer that samples the source region and
  // fits its destination at the resolved ratio draws every pixel at the shape it was measured
  // at. If the region's own display ratio ever drifted from the resolved one, the fit would
  // be a stretch.
  const auto shotRatio = [](const GeometryInputs& inputs)
  {
    const EffectiveGeometry geometry = ResolveEffectiveGeometry(inputs);
    const CRect coded{
        static_cast<float>(geometry.codedFrame.x1), static_cast<float>(geometry.codedFrame.y1),
        static_cast<float>(geometry.codedFrame.x2), static_cast<float>(geometry.codedFrame.y2)};
    const CRect source = SourceRect(RenderGeometryOf(geometry), coded);
    return std::pair<float, float>{source.Width() * geometry.par / source.Height(),
                                   geometry.aspect};
  };

  GeometryInputs letterboxed;
  letterboxed.stream = Uhd();
  letterboxed.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 280, 3840, 1880});
  auto [cut, resolved] = shotRatio(letterboxed);
  EXPECT_NEAR(resolved, cut, 0.001f);

  GeometryInputs anamorphic = ScopeCachedPal();
  std::tie(cut, resolved) = shotRatio(anamorphic);
  EXPECT_NEAR(resolved, cut, 0.001f);

  // Turned, the resolved ratio is the upright one and the coded region is upright too.
  GeometryInputs turned;
  turned.stream = Uhd(90);
  turned.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 280, 3840, 1880});
  std::tie(cut, resolved) = shotRatio(turned);
  EXPECT_NEAR(resolved, cut, 0.001f);
}

/*!
 * The guard the render path cuts behind. A resolved geometry outlives the frame it came from,
 * and on a stereoscopic stream the region rendered is one view - so the check is against the
 * rectangle being drawn, not the packing that holds two of it. Asked against the packing, a
 * half side-by-side title never matches and its bars are never discarded.
 */
TEST(TestEffectiveGeometry, AGeometryDescribesTheRegionItWasMeasuredIn)
{
  GeometryInputs inputs;
  inputs.stream = MeasuredStreamGeometry("left_right", 1920, 1080, 16.0f / 9.0f);
  inputs.cached = Cached(CRectInt{0, 132, 960, 948}, CRectInt{0, 132, 960, 948});

  const RenderGeometry view{RenderGeometryOf(ResolveEffectiveGeometry(inputs))};

  EXPECT_TRUE(DescribesFrame(view, CRect{0.0f, 0.0f, 960.0f, 1080.0f}));
  EXPECT_FALSE(DescribesFrame(view, CRect{0.0f, 0.0f, 1920.0f, 1080.0f}))
      << "the packed frame is not the region the measurement describes";

  // The rendered region is a rectangle, not necessarily one at the origin: the right-hand view
  // of a side-by-side frame starts halfway across and is still the region that was measured.
  EXPECT_TRUE(DescribesFrame(view, CRect{960.0f, 0.0f, 1920.0f, 1080.0f}));

  // Nothing resolved yet, so there is no rectangle to cut to whatever the frame says. A default
  // geometry has a zero frame, which would otherwise match a zero source.
  EXPECT_FALSE(DescribesFrame(RenderGeometry{}, CRect{0.0f, 0.0f, 960.0f, 1080.0f}));
  EXPECT_FALSE(DescribesFrame(RenderGeometry{}, CRect{}));
}

TEST(TestEffectiveGeometry, ARendererDrawingTheWholeFrameAgreesWithPictureRect)
{
  GeometryInputs inputs = ScopeCachedUhd();
  const EffectiveGeometry geometry = ResolveEffectiveGeometry(inputs);

  // The two answers describe the same screen, so a consumer switching from one to the other
  // must not see the picture move.
  const CRect dest{0.0f, 0.0f, 1920.0f, 1080.0f};
  const CRect fromFrame = PictureRect(geometry, dest);
  const CRect fromDrawn =
      PictureOnScreen(RenderGeometryOf(geometry), CRect{0.0f, 0.0f, 3840.0f, 2160.0f}, dest);
  ExpectRect(fromDrawn, fromFrame.x1, fromFrame.y1, fromFrame.x2, fromFrame.y2);
  ExpectRect(fromDrawn, 0.0f, 140.0f, 1920.0f, 940.0f);
}

TEST(TestEffectiveGeometry, ASourceCutToTheContentPutsThePictureEverywhereItIsDrawn)
{
  GeometryInputs inputs = ScopeCachedUhd();

  // The renderer has already discarded the bars, so its destination holds nothing but
  // content, and insetting it by the frame's bars again would count them twice.
  ExpectRect(PictureOnScreen(RenderGeometryOf(ResolveEffectiveGeometry(inputs)),
                             CRect{0.0f, 280.0f, 3840.0f, 1880.0f},
                             CRect{0.0f, 280.0f, 2560.0f, 1350.0f}),
             0.0f, 280.0f, 2560.0f, 1350.0f);
}

TEST(TestEffectiveGeometry, OnlyTheDrawnPartOfTheContentCounts)
{
  GeometryInputs inputs = ScopeCachedUhd();

  // The top half of the frame is drawn into a half-height destination: the bar is 280 of the
  // 1080 drawn, and the content runs off the bottom edge with the frame.
  ExpectRect(PictureOnScreen(RenderGeometryOf(ResolveEffectiveGeometry(inputs)),
                             CRect{0.0f, 0.0f, 3840.0f, 1080.0f},
                             CRect{0.0f, 0.0f, 1920.0f, 540.0f}),
             0.0f, 140.0f, 1920.0f, 540.0f);
}

TEST(TestEffectiveGeometry, ATurnedSourceIsMappedOntoTheTurnedDestination)
{
  GeometryInputs inputs;
  inputs.stream = Uhd(90);
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 280, 3840, 1880});
  const EffectiveGeometry geometry = ResolveEffectiveGeometry(inputs);

  // The same screen as the PictureRect case above: the source is coded and upright, the
  // destination is portrait, and the turn between them is applied on the way through.
  const CRect dest{660.0f, 0.0f, 1260.0f, 1080.0f};
  const CRect fromFrame = PictureRect(geometry, dest);
  const CRect fromDrawn =
      PictureOnScreen(RenderGeometryOf(geometry), CRect{0.0f, 0.0f, 3840.0f, 2160.0f}, dest);
  ExpectRect(fromDrawn, fromFrame.x1, fromFrame.y1, fromFrame.x2, fromFrame.y2);
}

TEST(TestEffectiveGeometry, PictureOnScreenWithNothingKnownIsTheDestination)
{
  const CRect dest{0.0f, 0.0f, 1920.0f, 1080.0f};
  ExpectRect(PictureOnScreen(RenderGeometry{}, CRect{0.0f, 0.0f, 3840.0f, 2160.0f}, dest), 0.0f,
             0.0f, 1920.0f, 1080.0f);
}

/*!
 * A title with one presentation ratio and scenes composed narrower inside it - a director
 * framing a shot, not the film changing shape. Following one brings the masking in for the
 * scene and out again after it, which is the room rearranging itself around the cutting.
 */
TEST(TestEffectiveGeometry, AReadingNarrowerThanWhatIsServedDoesNotMoveTheRoom)
{
  EXPECT_FALSE(LiveReadingWidens(1.90f, 2.40f)) << "a scene composed narrower moved the room";
  EXPECT_FALSE(LiveReadingWidens(1.78f, 2.40f));
}

//! \brief The other half: a title opening on an ident before it settles has to open the masking,
//! or it plays the whole film inside the ident's shape.
TEST(TestEffectiveGeometry, AReadingWiderThanWhatIsServedOpensTheRoom)
{
  EXPECT_TRUE(LiveReadingWidens(2.40f, 1.78f));

  // Nothing served yet, which is a title with no stored measurement on its first reading.
  EXPECT_TRUE(LiveReadingWidens(1.78f, 0.0f));
}

/*!
 * The detector wanders by a row or two between frames on the same shot. Without a floor those
 * readings ratchet the masking open a pixel at a time for the length of the film.
 */
TEST(TestEffectiveGeometry, DetectorNoiseAroundTheServedRatioDoesNotRatchetItOpen)
{
  EXPECT_FALSE(LiveReadingWidens(2.4001f, 2.40f));
  EXPECT_FALSE(LiveReadingWidens(2.41f, 2.40f));

  // The vocabulary's closest neighbouring pair still has to get through.
  EXPECT_TRUE(LiveReadingWidens(2.00f, 1.85f));
}
