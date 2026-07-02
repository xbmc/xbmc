/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/ILocalizer.h"
#include "utils/i18n/ListFormatter.h"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

namespace
{
// Bidi isolate markers (UAX #9). Explicit UTF-8 bytes avoid execution-charset issues.
constexpr std::string_view FSI = "\xE2\x81\xA8"; // U+2068 First Strong Isolate
constexpr std::string_view PDI = "\xE2\x81\xA9"; // U+2069 Pop Directional Isolate

// Arabic (RTL)
constexpr std::string_view AR_AND = "\xD9\x88"; // و   U+0648 (waw, "and")
constexpr std::string_view AR_COMMA = "\xD8\x8C"; // ،   U+060C (Arabic comma)
constexpr std::string_view AR_A = "\xD8\xA7\xD8\xA8"; // اب
constexpr std::string_view AR_B = "\xD8\xA8\xD8\xAA"; // بت
constexpr std::string_view AR_C = "\xD8\xAA\xD8\xA7"; // تا

//! \brief Wrap text in First-Strong/Pop directional isolates.
std::string Iso(std::string_view s)
{
  std::string out;
  out.reserve(FSI.size() + s.size() + PDI.size());
  out.append(FSI).append(s).append(PDI);
  return out;
}

std::string Sv(std::string_view s)
{
  return std::string(s);
}

size_t CountSubstr(const std::string& haystack, std::string_view needle)
{
  size_t count = 0;
  for (size_t pos = haystack.find(needle); pos != std::string::npos;
       pos = haystack.find(needle, pos + needle.size()))
    ++count;
  return count;
}

// Arabic "and" list patterns: two="{0} و{1}", sep="{0}، {1}", end="{0}، و{1}".
CListFormatter::Patterns ArabicAndPatterns()
{
  return {
      Sv("{0} ").append(AR_AND).append("{1}"), // two
      Sv("{0}").append(AR_COMMA).append(" {1}"), // start
      Sv("{0}").append(AR_COMMA).append(" {1}"), // middle
      Sv("{0}").append(AR_COMMA).append(" ").append(AR_AND).append("{1}"), // end
  };
}

// English "and" list patterns (modern CLDR, Oxford comma): "a, b, and c".
CListFormatter::Patterns EnglishAndPatterns()
{
  return {"{0} and {1}", "{0}, {1}", "{0}, {1}", "{0}, and {1}"};
}

//! \brief Returns a fixed string for any string id (proves the factory reads the localizer).
class CAlwaysLocalizer : public ILocalizer
{
public:
  explicit CAlwaysLocalizer(std::string value) : m_value(std::move(value)) {}
  std::string Localize(std::uint32_t /*code*/) const override { return m_value; }

private:
  std::string m_value;
};

//! \brief Returns empty for every id, forcing CListFormatter's English fallbacks.
class CEmptyLocalizer : public ILocalizer
{
public:
  std::string Localize(std::uint32_t /*code*/) const override { return {}; }
};
} // unnamed namespace

// --------------------------------------------------------------------------
// Structural / item-count cases
// --------------------------------------------------------------------------

TEST(ListFormatter, Empty_ReturnsEmpty)
{
  const CListFormatter fmt(ArabicAndPatterns());
  EXPECT_EQ(fmt.Format({}), "");
}

TEST(ListFormatter, SingleItem_IsIsolated)
{
  // A lone item must still be isolated so it is self-contained when embedded
  // inside surrounding UI text of the opposite direction.
  const CListFormatter fmt(ArabicAndPatterns());
  EXPECT_EQ(fmt.Format({Sv(AR_A)}), Iso(AR_A));
}

// --------------------------------------------------------------------------
// Pure RTL content - logical order is preserved (NOT visually reversed)
// --------------------------------------------------------------------------

TEST(ListFormatter, Arabic_And_TwoItems)
{
  const CListFormatter fmt(ArabicAndPatterns());
  const std::string expected = Iso(AR_A) + " " + Sv(AR_AND) + Iso(AR_B);
  EXPECT_EQ(fmt.Format({Sv(AR_A), Sv(AR_B)}), expected);
}

TEST(ListFormatter, Arabic_And_ThreeItems_LogicalOrderWithIsolates)
{
  const CListFormatter fmt(ArabicAndPatterns());
  const std::string expected =
      Iso(AR_A) + Sv(AR_COMMA) + " " + Iso(AR_B) + Sv(AR_COMMA) + " " + Sv(AR_AND) + Iso(AR_C);
  EXPECT_EQ(fmt.Format({Sv(AR_A), Sv(AR_B), Sv(AR_C)}), expected);
}

// --------------------------------------------------------------------------
// Mixed-direction lists
// --------------------------------------------------------------------------

TEST(ListFormatter, MixedDirection_EachItemIsolated)
{
  // Arabic ("and") list containing one LTR/Latin item. Every item - including
  // the Latin one - must be wrapped so the UBA cannot leak across the
  // item/separator boundary.
  const CListFormatter fmt(ArabicAndPatterns());
  const std::string result = fmt.Format({Sv(AR_A), "Bob", Sv(AR_C)});

  const std::string expected =
      Iso(AR_A) + Sv(AR_COMMA) + " " + Iso("Bob") + Sv(AR_COMMA) + " " + Sv(AR_AND) + Iso(AR_C);
  EXPECT_EQ(result, expected);

  // The Latin item is bracketed by FSI...PDI.
  EXPECT_NE(result.find(Iso("Bob")), std::string::npos);
}

TEST(ListFormatter, IsolationDisabled_NoMarkers)
{
  // With isolation off the mixed list contains no isolates
  // and is therefore vulnerable to UBA reordering at render time.
  const CListFormatter fmt(ArabicAndPatterns(), /*isolateItems*/ false);
  const std::string result = fmt.Format({Sv(AR_A), "Bob", Sv(AR_C)});

  EXPECT_EQ(CountSubstr(result, FSI), 0u);
  EXPECT_EQ(CountSubstr(result, PDI), 0u);
  EXPECT_EQ(result, Sv(AR_A) + Sv(AR_COMMA) + " Bob" + Sv(AR_COMMA) + " " + Sv(AR_AND) + Sv(AR_C));
}

// --------------------------------------------------------------------------
// Isolation invariants
// --------------------------------------------------------------------------

TEST(ListFormatter, IsolationCountMatchesItemCount)
{
  const CListFormatter fmt(ArabicAndPatterns());

  EXPECT_EQ(CountSubstr(fmt.Format({}), FSI), 0u);
  EXPECT_EQ(CountSubstr(fmt.Format({Sv(AR_A)}), FSI), 1u);
  EXPECT_EQ(CountSubstr(fmt.Format({Sv(AR_A), Sv(AR_B)}), FSI), 2u);

  const std::string three = fmt.Format({Sv(AR_A), Sv(AR_B), Sv(AR_C)});
  EXPECT_EQ(CountSubstr(three, FSI), 3u); // one open per item
  EXPECT_EQ(CountSubstr(three, PDI), 3u); // one close per item
}

TEST(ListFormatter, ConjunctionIsNotIsolated)
{
  // Pattern literals (separators / conjunction) must sit OUTSIDE the isolates.
  const CListFormatter fmt(ArabicAndPatterns());
  const std::string result = fmt.Format({Sv(AR_A), Sv(AR_B), Sv(AR_C)});

  // "and" should never appear wrapped as FSI و PDI.
  EXPECT_EQ(CountSubstr(result, Iso(AR_AND)), 0u);
  // The conjunction is immediately followed by the next item's opening isolate.
  EXPECT_NE(result.find(Sv(AR_AND) + Sv(FSI)), std::string::npos);
}

TEST(ListFormatter, ItemContainingPlaceholderIsNotReExpanded)
{
  // Single-pass substitution: an item that itself contains "{1}" is treated as
  // data, not as a pattern placeholder.
  const CListFormatter::Patterns en{"{0} and {1}", "{0}, {1}", "{0}, {1}", "{0}, and {1}"};
  const CListFormatter fmt(en, /*isolateItems*/ false);
  EXPECT_EQ(fmt.Format({"{1}", "x"}), "{1} and x");
}

TEST(ListFormatter, Units_NoConjunction_StillIsolates)
{
  // UNITS-style: separators only, no conjunction, but items remain isolated.
  const CListFormatter::Patterns units{"{0}, {1}", "{0}, {1}", "{0}, {1}", "{0}, {1}"};
  const CListFormatter fmt(units);
  EXPECT_EQ(fmt.Format({Sv(AR_A), Sv(AR_B), Sv(AR_C)}),
            Iso(AR_A) + ", " + Iso(AR_B) + ", " + Iso(AR_C));
}

// --------------------------------------------------------------------------
// CreateInstance: localization wiring + English fallback
// --------------------------------------------------------------------------

//! \brief Returns a configurable string per string id; falls back to empty.
namespace
{
class CMapLocalizer : public ILocalizer
{
public:
  explicit CMapLocalizer(std::map<std::uint32_t, std::string> strings)
    : m_strings(std::move(strings))
  {
  }
  std::string Localize(std::uint32_t code) const override
  {
    const auto it = m_strings.find(code);
    return it != m_strings.end() ? it->second : std::string{};
  }

private:
  std::map<std::uint32_t, std::string> m_strings;
};
} // unnamed namespace

TEST(ListFormatter, CreateInstance_FallbackEnglish_And)
{
  const CEmptyLocalizer localizer;
  const CListFormatter fmt = CListFormatter::CreateInstance(localizer, ListFormatType::AND);
  EXPECT_EQ(fmt.Format({"a", "b", "c"}), Iso("a") + ", " + Iso("b") + ", and " + Iso("c"));
}

TEST(ListFormatter, CreateInstance_FallbackEnglish_Or)
{
  const CEmptyLocalizer localizer;
  const CListFormatter fmt = CListFormatter::CreateInstance(localizer, ListFormatType::OR);
  EXPECT_EQ(fmt.Format({"a", "b"}), Iso("a") + " or " + Iso("b"));
}

TEST(ListFormatter, CreateInstance_Units_FallbackEnglish)
{
  // With no localized strings UNITS falls back to English ", " separator for
  // all slots. Output is identical to the old hard-coded path but now goes
  // through Localize() so locale overrides are honoured.
  const CEmptyLocalizer localizer;
  const CListFormatter fmt = CListFormatter::CreateInstance(localizer, ListFormatType::UNITS);
  EXPECT_EQ(fmt.Format({"a", "b", "c"}), Iso("a") + ", " + Iso("b") + ", " + Iso("c"));
}

TEST(ListFormatter, CreateInstance_Units_UsesLocalizedSeparator)
{
  // Prove CreateInstance reads the localizer for UNITS (previously bypassed).
  // CAlwaysLocalizer returns the Arabic comma separator for every string id.
  const CAlwaysLocalizer localizer(Sv("{0}").append(AR_COMMA).append(" {1}"));
  const CListFormatter fmt = CListFormatter::CreateInstance(localizer, ListFormatType::UNITS);
  EXPECT_EQ(fmt.Format({Sv(AR_A), Sv(AR_B), Sv(AR_C)}),
            Iso(AR_A) + Sv(AR_COMMA) + " " + Iso(AR_B) + Sv(AR_COMMA) + " " + Iso(AR_C));
}

TEST(ListFormatter, CreateInstance_Units_StartAndMiddleCanDiffer)
{
  // UNITS builds: two=start=STR_LIST_AND_START, middle=end=STR_LIST_AND_MIDDLE.
  // Use distinct patterns per ID so we can verify the slot assignment.
  const CMapLocalizer localizer({
      {STR_LIST_AND_START, "{0}~SEP_START~{1}"},
      {STR_LIST_AND_MIDDLE, "{0}~SEP_MIDDLE~{1}"},
  });
  const CListFormatter fmt =
      CListFormatter::CreateInstance(localizer, ListFormatType::UNITS, ListFormatWidth::WIDE,
                                     /*isolateItems=*/false);

  // Two items → two slot (= start pattern)
  EXPECT_EQ(fmt.Format({"a", "b"}), "a~SEP_START~b");
  // Three items → start then middle (= middle pattern for end, no conjunction)
  EXPECT_EQ(fmt.Format({"a", "b", "c"}), "a~SEP_START~b~SEP_MIDDLE~c");
  // Four items → start, middle (inner), middle (end)
  EXPECT_EQ(fmt.Format({"a", "b", "c", "d"}), "a~SEP_START~b~SEP_MIDDLE~c~SEP_MIDDLE~d");
}

TEST(ListFormatter, CreateInstance_ReadsLocalizedRtlPattern)
{
  // Proves the factory pulls patterns from the localizer (RTL "two" pattern),
  // rather than always using the English fallback.
  const CAlwaysLocalizer localizer(Sv("{0} ").append(AR_AND).append("{1}"));
  const CListFormatter fmt = CListFormatter::CreateInstance(localizer, ListFormatType::AND);
  EXPECT_EQ(fmt.Format({Sv(AR_A), Sv(AR_B)}), Iso(AR_A) + " " + Sv(AR_AND) + Iso(AR_B));
}

// --------------------------------------------------------------------------
// English / LTR content
// --------------------------------------------------------------------------

TEST(ListFormatter, English_SingleItem_IsIsolated)
{
  // Isolation is direction-agnostic: a lone LTR item is wrapped too.
  const CListFormatter fmt(EnglishAndPatterns());
  EXPECT_EQ(fmt.Format({"Alice"}), Iso("Alice"));
}

TEST(ListFormatter, English_And_TwoItems)
{
  const CListFormatter fmt(EnglishAndPatterns());
  EXPECT_EQ(fmt.Format({"Alice", "Bob"}), Iso("Alice") + " and " + Iso("Bob"));
}

TEST(ListFormatter, English_And_ThreeItems)
{
  const CListFormatter fmt(EnglishAndPatterns());
  EXPECT_EQ(fmt.Format({"a", "b", "c"}), Iso("a") + ", " + Iso("b") + ", and " + Iso("c"));
}

TEST(ListFormatter, English_And_FourItems_DocExample)
{
  // The icu::ListFormatter doc example. Four items is the smallest list that
  // exercises the `middle` pattern (items between first pair and last).
  const CListFormatter fmt(EnglishAndPatterns());
  const std::string expected =
      Iso("Alice") + ", " + Iso("Bob") + ", " + Iso("Charlie") + ", and " + Iso("Delta");
  EXPECT_EQ(fmt.Format({"Alice", "Bob", "Charlie", "Delta"}), expected);
}

TEST(ListFormatter, English_And_FourItems_DocExample_NoIsolation)
{
  // Human-readable canonical output (Oxford comma per modern CLDR).
  const CListFormatter fmt(EnglishAndPatterns(), /*isolateItems*/ false);
  EXPECT_EQ(fmt.Format({"Alice", "Bob", "Charlie", "Delta"}), "Alice, Bob, Charlie, and Delta");
}

TEST(ListFormatter, English_Or_ThreeItems)
{
  const CListFormatter::Patterns en{"{0} or {1}", "{0}, {1}", "{0}, {1}", "{0}, or {1}"};
  const CListFormatter fmt(en);
  EXPECT_EQ(fmt.Format({"a", "b", "c"}), Iso("a") + ", " + Iso("b") + ", or " + Iso("c"));
}

TEST(ListFormatter, French_And_LocalizedConjunction)
{
  // Non-English LTR: French uses "et" and no comma before the conjunction.
  const CListFormatter::Patterns fr{"{0} et {1}", "{0}, {1}", "{0}, {1}", "{0} et {1}"};
  const CListFormatter fmt(fr);
  EXPECT_EQ(fmt.Format({"a", "b", "c"}), Iso("a") + ", " + Iso("b") + " et " + Iso("c"));
}

TEST(ListFormatter, Ltr_WithEmbeddedRtlItem_EachItemIsolated)
{
  // Mirror of the Arabic mixed-direction case: an English (LTR) list that
  // contains an Arabic (RTL) item. The RTL item must be isolated so it cannot
  // disturb the surrounding LTR separators.
  const CListFormatter fmt(EnglishAndPatterns());
  const std::string result = fmt.Format({"Alice", Sv(AR_A), "Bob"});

  EXPECT_EQ(result, Iso("Alice") + ", " + Iso(AR_A) + ", and " + Iso("Bob"));
  EXPECT_NE(result.find(Iso(AR_A)), std::string::npos);
}

TEST(ListFormatter, StartMiddleEnd_AppliedAtCorrectPositions)
{
  // Distinct markers per slot prove each pattern is applied at the right
  // position: `two` only for exactly two items; otherwise start(front),
  // middle(inner, possibly repeated), end(back).
  const CListFormatter::Patterns p{"{0}~TWO~{1}", "{0}~START~{1}", "{0}~MIDDLE~{1}", "{0}~END~{1}"};
  const CListFormatter fmt(p, /*isolateItems*/ false);

  EXPECT_EQ(fmt.Format({"a", "b"}), "a~TWO~b");
  EXPECT_EQ(fmt.Format({"a", "b", "c"}), "a~START~b~END~c"); // no middle yet
  EXPECT_EQ(fmt.Format({"a", "b", "c", "d", "e"}), "a~START~b~MIDDLE~c~MIDDLE~d~END~e");
}

// --------------------------------------------------------------------------
// Marker sanity
// --------------------------------------------------------------------------

TEST(ListFormatter, IsolateMarkers_AreCorrectCodepoints)
{
  ASSERT_EQ(FSI.size(), 3u);
  ASSERT_EQ(PDI.size(), 3u);
  EXPECT_EQ(static_cast<unsigned char>(FSI[0]), 0xE2); // U+2068
  EXPECT_EQ(static_cast<unsigned char>(FSI[1]), 0x81);
  EXPECT_EQ(static_cast<unsigned char>(FSI[2]), 0xA8);
  EXPECT_EQ(static_cast<unsigned char>(PDI[2]), 0xA9); // U+2069
}
