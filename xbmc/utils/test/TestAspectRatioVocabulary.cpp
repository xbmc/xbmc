/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/AspectRatioVocabulary.h"

#include <cmath>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::UTILS;

namespace
{

//! The shipped definition, read through the same path and protocol handler Load() uses.
std::string ShippedDefinition()
{
  XFILE::CFile file;
  std::vector<uint8_t> content;
  if (file.LoadFile("special://xbmc/system/aspectratios.xml", content) <= 0)
    return {};

  return std::string(reinterpret_cast<const char*>(content.data()), content.size());
}

} // namespace

/*!
 * The vocabulary is the shipped definition and nothing else - no table in the code stands
 * behind it - so every test here runs against the file Kodi actually ships, and a definition
 * broken by an edit fails the suite rather than quietly changing what every installation
 * recognises.
 *
 * It is also process-wide by design, so a test that leaves one applied changes the answers
 * every later test gets. Putting it back is the fixture's other job.
 */
class TestAspectRatioVocabulary : public testing::Test
{
protected:
  void SetUp() override
  {
    CAspectRatioVocabulary::Reset();
    ASSERT_TRUE(CAspectRatioVocabulary::Apply(ShippedDefinition()));
  }

  void TearDown() override { CAspectRatioVocabulary::Reset(); }
};

TEST_F(TestAspectRatioVocabulary, EntriesAreAscending)
{
  //! Nearest() cuts between adjacent entries, so order is a correctness requirement.
  const auto entries = CAspectRatioVocabulary::Entries();
  ASSERT_FALSE(entries.empty());

  for (size_t i = 1; i < entries.size(); ++i)
    EXPECT_LT(entries[i - 1].ratio, entries[i].ratio);
}

TEST_F(TestAspectRatioVocabulary, EveryEntryIsDeclarable)
{
  //! A declaration is never second-guessed, so nothing Kodi knows about is withheld from it.
  for (const auto& entry : CAspectRatioVocabulary::Entries())
    EXPECT_TRUE(entry.declare) << entry.label;
}

TEST_F(TestAspectRatioVocabulary, EveryEntryClassifiesAsItself)
{
  for (const auto& entry : CAspectRatioVocabulary::Entries())
  {
    const auto nearest = CAspectRatioVocabulary::Nearest(entry.ratio);
    ASSERT_TRUE(nearest.has_value()) << entry.label;
    EXPECT_EQ(entry.label, nearest->label);
  }
}

TEST_F(TestAspectRatioVocabulary, EveryLabelIsTheRatioWrittenOut)
{
  //! Derived rather than stored, so a definition file cannot put words where every existing
  //! consumer - ListItem.VideoAspect, the library, JSON-RPC - expects the number.
  for (const auto& entry : CAspectRatioVocabulary::Entries())
    EXPECT_EQ(entry.label, CAspectRatioVocabulary::Label(entry.ratio));
}

TEST_F(TestAspectRatioVocabulary, NearestNeverRejects)
{
  //! Anything beyond the widest entry is still classified as it. Match() is the check that
  //! rejects; this one deliberately does not.
  const auto nearest = CAspectRatioVocabulary::Nearest(4.5f);
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ("2.76", nearest->label);
}

TEST_F(TestAspectRatioVocabulary, NearestRejectsNonRatios)
{
  EXPECT_FALSE(CAspectRatioVocabulary::Nearest(0.0f).has_value());
  EXPECT_FALSE(CAspectRatioVocabulary::Nearest(-1.0f).has_value());
  EXPECT_TRUE(CAspectRatioVocabulary::Label(0.0f).empty());
  EXPECT_TRUE(CAspectRatioVocabulary::Label(-1.0f).empty());
}

TEST_F(TestAspectRatioVocabulary, NearestCutsAtTheGeometricMean)
{
  //! Probed either side of each cutoff so a change to the table cannot silently move one.
  //! These are the same values the classifier is pinned against.
  EXPECT_EQ("1.00", CAspectRatioVocabulary::Nearest(1.0899f)->label);
  EXPECT_EQ("1.19", CAspectRatioVocabulary::Nearest(1.0919f)->label);

  EXPECT_EQ("1.33", CAspectRatioVocabulary::Nearest(1.3505f)->label);
  EXPECT_EQ("1.37", CAspectRatioVocabulary::Nearest(1.3525f)->label);

  EXPECT_EQ("1.43", CAspectRatioVocabulary::Nearest(1.4636f)->label);
  EXPECT_EQ("1.50", CAspectRatioVocabulary::Nearest(1.4656f)->label);

  EXPECT_EQ("1.78", CAspectRatioVocabulary::Nearest(1.8125f)->label);
  EXPECT_EQ("1.85", CAspectRatioVocabulary::Nearest(1.8145f)->label);

  EXPECT_EQ("2.55", CAspectRatioVocabulary::Nearest(2.6519f)->label);
  EXPECT_EQ("2.76", CAspectRatioVocabulary::Nearest(2.6539f)->label);
}

TEST_F(TestAspectRatioVocabulary, RatiosThatAreExactFractionsAreHeldAsThemselves)
{
  //! 16:9 is 1.7778, and an entry at the two-decimal nominal would put every full-frame
  //! measurement and declaration a couple of pixels inside its own frame. The label and the
  //! key still read at two decimals, which is what every consumer stores and displays.
  EXPECT_FLOAT_EQ(16.0f / 9.0f, CAspectRatioVocabulary::RatioForKey(178));
  EXPECT_FLOAT_EQ(4.0f / 3.0f, CAspectRatioVocabulary::RatioForKey(133));
  EXPECT_EQ("1.78", CAspectRatioVocabulary::Label(16.0f / 9.0f));
  EXPECT_EQ("1.33", CAspectRatioVocabulary::Label(4.0f / 3.0f));
}

TEST_F(TestAspectRatioVocabulary, ResolveErrsTowardTheShapeTheRoomRestsAt)
{
  //! 3840x1620 is 2.3704, within tolerance of both 2.35 and 2.40. Which is right depends on
  //! the room: erring toward its resting shape opens a mask onto fabric, erring away spills
  //! picture onto the wall.
  const float ratio = 3840.0f / 1620.0f;

  const auto towardFlat = CAspectRatioVocabulary::Resolve(ratio, AspectRatioUse::Detect, 1.78f);
  ASSERT_TRUE(towardFlat.has_value());
  EXPECT_EQ("2.35", towardFlat->label);

  const auto towardScope = CAspectRatioVocabulary::Resolve(ratio, AspectRatioUse::Detect, 2.40f);
  ASSERT_TRUE(towardScope.has_value());
  EXPECT_EQ("2.40", towardScope->label);
}

TEST_F(TestAspectRatioVocabulary, ResolveRejectsExactlyAsMatchDoes)
{
  //! The direction rule chooses between entries a measurement could support; it must never
  //! rescue one that supports none.
  EXPECT_FALSE(CAspectRatioVocabulary::Resolve(1.60f, AspectRatioUse::Detect, 1.78f).has_value());
  EXPECT_FALSE(CAspectRatioVocabulary::Resolve(4.5f, AspectRatioUse::Detect, 2.76f).has_value());
  EXPECT_FALSE(CAspectRatioVocabulary::Resolve(0.0f, AspectRatioUse::Detect, 1.78f).has_value());
}

TEST_F(TestAspectRatioVocabulary, ResolveCannotBePulledBeyondTheTolerance)
{
  //! 2.33 is within tolerance of 2.35 alone; a room resting at 2.76 must not drag it to an
  //! entry the measurement cannot support.
  const auto resolved = CAspectRatioVocabulary::Resolve(2.33f, AspectRatioUse::Detect, 2.76f);
  ASSERT_TRUE(resolved.has_value());
  EXPECT_EQ("2.35", resolved->label);
}

TEST_F(TestAspectRatioVocabulary, ResolveWithNoRestingShapeIsMatch)
{
  const float ratio = 3840.0f / 1620.0f; // nearer 2.35 than 2.40 in log space
  const auto resolved = CAspectRatioVocabulary::Resolve(ratio, AspectRatioUse::Detect, 0.0f);
  ASSERT_TRUE(resolved.has_value());
  EXPECT_EQ("2.35", resolved->label);
  EXPECT_EQ(CAspectRatioVocabulary::Match(ratio, AspectRatioUse::Detect)->label, resolved->label);
}

TEST_F(TestAspectRatioVocabulary, LabelMatchesNearest)
{
  EXPECT_EQ("1.78", CAspectRatioVocabulary::Label(1920.0f / 1080.0f));
  EXPECT_EQ("2.35", CAspectRatioVocabulary::Label(3840.0f / 1632.0f));
}

TEST_F(TestAspectRatioVocabulary, NameIsServedBesideTheLabelAndMayBeEmpty)
{
  //! The two are separate values on purpose: a consumer branches on one and displays the
  //! other, and a ratio with no established name must not fall back to the number.
  EXPECT_EQ("16:9", CAspectRatioVocabulary::Name(1920.0f / 1080.0f));
  EXPECT_EQ("1.78", CAspectRatioVocabulary::Label(1920.0f / 1080.0f));

  EXPECT_TRUE(CAspectRatioVocabulary::Name(1.66f).empty());
  EXPECT_EQ("1.66", CAspectRatioVocabulary::Label(1.66f));

  EXPECT_TRUE(CAspectRatioVocabulary::Name(0.0f).empty());
}

TEST_F(TestAspectRatioVocabulary, DistanceIsSymmetricAndZeroForEqual)
{
  EXPECT_FLOAT_EQ(0.0f, CAspectRatioVocabulary::Distance(1.78f, 1.78f));
  EXPECT_FLOAT_EQ(CAspectRatioVocabulary::Distance(1.78f, 2.40f),
                  CAspectRatioVocabulary::Distance(2.40f, 1.78f));
}

TEST_F(TestAspectRatioVocabulary, DistanceRejectsNonRatios)
{
  EXPECT_TRUE(std::isinf(CAspectRatioVocabulary::Distance(0.0f, 1.78f)));
  EXPECT_TRUE(std::isinf(CAspectRatioVocabulary::Distance(1.78f, -1.0f)));
}

TEST_F(TestAspectRatioVocabulary, MatchAcceptsOffNominalEncodes)
{
  //! 1888x1080 is 1.748, 1.7% from the 1.78 entry, and is a correct encode rather than a
  //! measurement error. A tolerance set to the width of a rounding error rejects it.
  const auto match = CAspectRatioVocabulary::Match(1888.0f / 1080.0f);
  ASSERT_TRUE(match.has_value());
  EXPECT_EQ("1.78", match->label);
}

TEST_F(TestAspectRatioVocabulary, MatchRejectsRatiosBetweenEntries)
{
  EXPECT_FALSE(CAspectRatioVocabulary::Match(1.60f).has_value());
  EXPECT_FALSE(CAspectRatioVocabulary::Match(2.10f).has_value());
}

TEST_F(TestAspectRatioVocabulary, MatchRejectsImplausibleRatios)
{
  //! The case Nearest() answers "2.76" to.
  EXPECT_FALSE(CAspectRatioVocabulary::Match(4.5f).has_value());
  EXPECT_FALSE(CAspectRatioVocabulary::Match(0.5f).has_value());
}

TEST_F(TestAspectRatioVocabulary, MatchNeverRejectsBetweenTheClosestNeighbours)
{
  //! 2.35 and 2.40 are 2.1% apart, so at the default tolerance there is no gap between them
  //! wide enough to fall into. Pinned so that this is a known consequence of the tolerance
  //! rather than a surprise to a caller treating a match as confirmation.
  const float midpoint = std::sqrt(2.35f * 2.40f);
  EXPECT_TRUE(CAspectRatioVocabulary::Match(midpoint).has_value());
}

TEST_F(TestAspectRatioVocabulary, MatchHonoursAnExplicitTolerance)
{
  const float ratio = 1888.0f / 1080.0f;
  EXPECT_TRUE(CAspectRatioVocabulary::Match(ratio, AspectRatioUse::Any, 0.02f).has_value());
  EXPECT_FALSE(CAspectRatioVocabulary::Match(ratio, AspectRatioUse::Any, 0.015f).has_value());
}

TEST_F(TestAspectRatioVocabulary, DetectionExcludesRatiosItCannotDistinguish)
{
  const auto detectable = CAspectRatioVocabulary::EntriesFor(AspectRatioUse::Detect);
  const auto declarable = CAspectRatioVocabulary::EntriesFor(AspectRatioUse::Declare);

  EXPECT_LT(detectable.size(), declarable.size());
  EXPECT_EQ(CAspectRatioVocabulary::Entries().size(), declarable.size());

  for (const auto& entry : detectable)
    EXPECT_TRUE(entry.detect) << entry.label;
}

TEST_F(TestAspectRatioVocabulary, ADetectedRatioNeverLandsOnANonDetectableEntry)
{
  //! A measurement at 2.76 is classified against the detectable entries only, so it resolves
  //! to 2.40 and is then rejected as too far from it. Declaring 2.76 remains available.
  const auto nearest = CAspectRatioVocabulary::Nearest(2.76f, AspectRatioUse::Detect);
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ("2.40", nearest->label);

  EXPECT_FALSE(CAspectRatioVocabulary::Match(2.76f, AspectRatioUse::Detect).has_value());
  EXPECT_TRUE(CAspectRatioVocabulary::Match(2.76f, AspectRatioUse::Declare).has_value());
}

TEST_F(TestAspectRatioVocabulary, AnEntryIsReplacedByValueRatherThanAppended)
{
  const size_t before = CAspectRatioVocabulary::Entries().size();

  ASSERT_TRUE(CAspectRatioVocabulary::Apply(
      R"(<aspectratios><ratio value="1.50" detect="true"/></aspectratios>)"));

  EXPECT_EQ(before, CAspectRatioVocabulary::Entries().size());

  const auto nearest = CAspectRatioVocabulary::Nearest(1.50f, AspectRatioUse::Detect);
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ("1.50", nearest->label) << "1.50 is now an answer a measurement may resolve to";
}

TEST_F(TestAspectRatioVocabulary, AValueMatchesAnEntryAtTwoDecimals)
{
  //! The built-in 16:9 entry is the exact fraction, and a viewer's file saying 1.78 must
  //! change it rather than stand beside it - two entries inside the same hundredth would
  //! share a key, leaving one of them unreachable from any stored choice.
  const size_t before = CAspectRatioVocabulary::Entries().size();

  ASSERT_TRUE(CAspectRatioVocabulary::Apply(
      R"(<aspectratios><ratio value="1.78" name="Widescreen"/></aspectratios>)"));

  EXPECT_EQ(before, CAspectRatioVocabulary::Entries().size());

  const auto entry = CAspectRatioVocabulary::Nearest(16.0f / 9.0f);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ("Widescreen", entry->name);
  EXPECT_FLOAT_EQ(16.0f / 9.0f, entry->ratio) << "the entry keeps its exact ratio";
}

TEST_F(TestAspectRatioVocabulary, AnAbsentAttributeKeepsWhatTheEntryHad)
{
  //! The whole point of merging: changing one property must not silently reset the others.
  ASSERT_TRUE(CAspectRatioVocabulary::Apply(
      R"(<aspectratios><ratio value="2.76" name="Ultra Panavision"/></aspectratios>)"));

  const auto nearest = CAspectRatioVocabulary::Nearest(2.76f);
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ("Ultra Panavision", nearest->name);
  EXPECT_FALSE(nearest->detect) << "2.76 was not detectable and nothing said to change that";
  EXPECT_TRUE(nearest->declare);
}

TEST_F(TestAspectRatioVocabulary, ANewRatioIsAddedInOrderAndLabelledFromItsValue)
{
  //! Sorted here rather than demanded of the file, because classification cuts between
  //! neighbours and a viewer adding a ratio should not have to know that.
  //! Custom delimiter: the name carries brackets, and a plain R"( ... )" would be closed by
  //! the )" this one contains.
  ASSERT_TRUE(CAspectRatioVocabulary::Apply(
      R"xml(<aspectratios><ratio value="2.66" name="Cinerama (three-strip)"/></aspectratios>)xml"));

  const auto entries = CAspectRatioVocabulary::Entries();
  for (size_t i = 1; i < entries.size(); ++i)
    EXPECT_LT(entries[i - 1].ratio, entries[i].ratio);

  const auto nearest = CAspectRatioVocabulary::Nearest(2.66f);
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ("2.66", nearest->label);
  EXPECT_EQ("Cinerama (three-strip)", nearest->name);
}

TEST_F(TestAspectRatioVocabulary, ToleranceComesFromTheDefinition)
{
  const float ratio = 1888.0f / 1080.0f; // 1.748, 1.7% from the 1.78 entry

  ASSERT_TRUE(CAspectRatioVocabulary::Apply(R"(<aspectratios tolerance="0.015"/>)"));
  EXPECT_FLOAT_EQ(0.015f, CAspectRatioVocabulary::Tolerance());
  EXPECT_FALSE(CAspectRatioVocabulary::Match(ratio).has_value())
      << "the tolerance in force has to reach a caller that did not pass one";
}

TEST_F(TestAspectRatioVocabulary, ABadDefinitionChangesNothingAtAll)
{
  const auto before = CAspectRatioVocabulary::Entries();

  //! Rejected after a good entry has already been merged, which is the case that would leave
  //! a half-applied vocabulary if the merge were done in place. Held in a variable because a
  //! raw string spanning lines cannot be passed straight into a macro.
  constexpr const char* definition =
      R"(<aspectratios><ratio value="2.66" name="valid"/><ratio value="notanumber"/></aspectratios>)";
  EXPECT_FALSE(CAspectRatioVocabulary::Apply(definition));

  const auto after = CAspectRatioVocabulary::Entries();
  ASSERT_EQ(before.size(), after.size());
  for (size_t i = 0; i < before.size(); ++i)
    EXPECT_FLOAT_EQ(before[i].ratio, after[i].ratio);
}

TEST_F(TestAspectRatioVocabulary, AMalformedDocumentIsRejected)
{
  EXPECT_FALSE(CAspectRatioVocabulary::Apply("this is not xml at all"));
  EXPECT_FALSE(
      CAspectRatioVocabulary::Apply(R"(<something-else><ratio value="2"/></something-else>)"));
  EXPECT_FALSE(CAspectRatioVocabulary::Apply(R"(<aspectratios><ratio value="0"/></aspectratios>)"));
  EXPECT_FALSE(CAspectRatioVocabulary::Apply(
      R"(<aspectratios><ratio value="1.78" detect="yes"/></aspectratios>)"));
  EXPECT_FALSE(CAspectRatioVocabulary::Apply(R"(<aspectratios tolerance="0"/>)"));

  //! The vocabulary still answers, which is the property everything else depends on.
  EXPECT_EQ("1.78", CAspectRatioVocabulary::Label(1920.0f / 1080.0f));
}

TEST_F(TestAspectRatioVocabulary, EveryEntryRoundTripsThroughItsKey)
{
  //! A setting and a list control both store the choice as this integer, so an entry that does
  //! not come back is an entry nobody can pick.
  for (const auto& entry : CAspectRatioVocabulary::Entries())
  {
    const int key = CAspectRatioVocabulary::Key(entry.ratio);
    EXPECT_FLOAT_EQ(entry.ratio, CAspectRatioVocabulary::RatioForKey(key)) << entry.label;
  }
}

TEST_F(TestAspectRatioVocabulary, EveryKeyIsDistinct)
{
  //! Two entries sharing a key would make one of them unreachable from a stored choice.
  std::set<int> keys;
  for (const auto& entry : CAspectRatioVocabulary::Entries())
    EXPECT_TRUE(keys.insert(CAspectRatioVocabulary::Key(entry.ratio)).second) << entry.label;
}

TEST_F(TestAspectRatioVocabulary, KeyZeroIsNotARatio)
{
  //! The declaration list uses zero for "Auto", so nothing may resolve to it.
  EXPECT_FLOAT_EQ(0.0f, CAspectRatioVocabulary::RatioForKey(0));
  EXPECT_FLOAT_EQ(0.0f, CAspectRatioVocabulary::RatioForKey(201)) << "no entry at 2.01";
}

TEST_F(TestAspectRatioVocabulary, AChoiceReadsAsTheNumberAndTheName)
{
  const auto named = CAspectRatioVocabulary::Nearest(2.40f);
  ASSERT_TRUE(named.has_value());
  EXPECT_EQ("2.40 - Scope", CAspectRatioVocabulary::ChoiceLabel(*named));

  //! An unnamed ratio is a legitimate answer rather than an omission, and reads as the bare
  //! number rather than as a number with an empty tail.
  const auto unnamed = CAspectRatioVocabulary::Nearest(1.66f);
  ASSERT_TRUE(unnamed.has_value());
  ASSERT_TRUE(unnamed->name.empty());
  EXPECT_EQ("1.66", CAspectRatioVocabulary::ChoiceLabel(*unnamed));
}

TEST_F(TestAspectRatioVocabulary, AKeyResolvesToTheDefinitionInForce)
{
  //! A ratio a viewer added is choosable, which is the whole point of the key being derived
  //! from the vocabulary rather than from a fixed list.
  ASSERT_TRUE(CAspectRatioVocabulary::Apply(
      R"(<aspectratios><ratio value="2.66" name="Cinerama"/></aspectratios>)"));

  EXPECT_FLOAT_EQ(2.66f, CAspectRatioVocabulary::RatioForKey(266));
}

TEST_F(TestAspectRatioVocabulary, ResetLeavesNothingToClassifyAgainst)
{
  //! Nothing stands behind a definition, so discarding one discards the vocabulary. Kodi then
  //! recognises no ratio rather than a different set from the one in the tree, which is the
  //! failure a reader can act on.
  CAspectRatioVocabulary::Reset();

  EXPECT_TRUE(CAspectRatioVocabulary::Entries().empty());
  EXPECT_FALSE(CAspectRatioVocabulary::Nearest(16.0f / 9.0f).has_value());
  EXPECT_FLOAT_EQ(CAspectRatioVocabulary::DEFAULT_TOLERANCE, CAspectRatioVocabulary::Tolerance());
}

TEST_F(TestAspectRatioVocabulary, ARatioAViewerAddsIsDeclarableButNotDetectable)
{
  //! Kodi has no evidence about a ratio it did not ship, and a wrong detection crops the
  //! picture - so an added ratio can be picked by hand immediately and becomes an answer a
  //! measurement may resolve to only when the file says so.
  ASSERT_TRUE(CAspectRatioVocabulary::Apply(
      R"(<aspectratios><ratio value="2.66" name="Cinerama"/></aspectratios>)"));

  const auto added = CAspectRatioVocabulary::Nearest(2.66f);
  ASSERT_TRUE(added.has_value());
  EXPECT_EQ("2.66", added->label);
  EXPECT_TRUE(added->declare);
  EXPECT_FALSE(added->detect);

  ASSERT_TRUE(CAspectRatioVocabulary::Apply(
      R"(<aspectratios><ratio value="2.66" detect="true"/></aspectratios>)"));
  EXPECT_TRUE(CAspectRatioVocabulary::Nearest(2.66f)->detect) << "and the name it already had";
  EXPECT_EQ("Cinerama", CAspectRatioVocabulary::Nearest(2.66f)->name);
}

/*!
 * The file Kodi actually ships, parsed by the code that will parse it at startup.
 *
 * Every other test here reaches the shipped file through the fixture. This one reaches it the
 * long way so that the two things that can go wrong say which one did: the file is missing
 * from where Load() looks, or it is there and does not parse. Load() reports either only in
 * the log, so without this the first sign would be an installation that recognises no ratio.
 */
TEST_F(TestAspectRatioVocabulary, TheShippedDefinitionParses)
{
  // The same path string Load() passes, resolved by the same protocol handler.
  const std::string path{"special://xbmc/system/aspectratios.xml"};

  XFILE::CFile file;
  std::vector<uint8_t> content;
  ASSERT_GT(file.LoadFile(path, content), 0)
      << "the shipped vocabulary is missing or empty where Load() looks for it: " << path;

  CAspectRatioVocabulary::Reset();
  ASSERT_TRUE(CAspectRatioVocabulary::Apply(
      std::string_view{reinterpret_cast<const char*>(content.data()), content.size()}))
      << "the shipped vocabulary does not parse, and Kodi would recognise no ratio at all";

  // It has to be a vocabulary, not merely well-formed XML: a document declaring no ratios
  // parses and leaves nothing to classify against.
  const auto entries = CAspectRatioVocabulary::Entries();
  EXPECT_FALSE(entries.empty());
  EXPECT_TRUE(CAspectRatioVocabulary::Match(16.0f / 9.0f).has_value())
      << "the shipped vocabulary does not recognise 16:9";
}
