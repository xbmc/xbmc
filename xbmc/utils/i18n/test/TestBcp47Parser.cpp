/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47Parser.h"
#include "utils/i18n/test/TestI18nUtils.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

struct TestBcp47Parser
{
  std::string input;
  ParsedBcp47Tag expected;
  bool status{true};
};

std::ostream& operator<<(std::ostream& os, const TestBcp47Parser& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestBcp47Parser ParseBcp47Tests[] = {
    // ISO 639-1 code
    {"ab", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {}, {}, {}, ""}},
    // ISO 639-2 code
    {"abc", {Bcp47TagType::WELL_FORMED, "abc", {}, "", "", {}, {}, {}, ""}},
    // ISO 639-2 code with extended language subtags
    {"abc-def-ghi", {Bcp47TagType::WELL_FORMED, "abc", {"def", "ghi"}, "", "", {}, {}, {}, ""}},
    // registered 5-8 letters code
    {"abcde", {Bcp47TagType::WELL_FORMED, "abcde", {}, "", "", {}, {}, {}, ""}},
    // invalid, more than 8 letters
    {"montenegro", {}, false},
    {"not a valid tag", {}, false},
    // script
    {"ab-abcd", {Bcp47TagType::WELL_FORMED, "ab", {}, "abcd", "", {}, {}, {}, ""}},
    {"ab-abc0", {}, false},
    // Region ISO 3166-1
    {"ab-ab", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "ab", {}, {}, {}, ""}},
    // Region UN M.49
    {"ab-012", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "012", {}, {}, {}, ""}},
    {"ab-a01", {}, false},
    // Variants
    {"ab-0abc", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {{"0abc"}}, {}, {}, ""}},
    {"ab-abcde-bcdefghi", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {{"abcde"}, {"bcdefghi"}}, {}, {}, ""}},
    {"ab-abcde-0abc-1def", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {{"abcde"}, {"0abc"}, {"1def"}}, {}, {}, ""}},
    {"ab-abcdefghi", {}, false},
    // Extensions
    {"ab-a-bcdefghi-jk", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {}, {{'a', {{"bcdefghi"}, {"jk"}}}}, {}, ""}},
    {"ab-a-bc-de-a-fg-hi", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {}, {{'a', {{"bc"}, {"de"}}}, {'a', {{"fg"}, {"hi"}}}}, {}, ""}},
    {"ab-a-bc-d-ef", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {}, {{'a', {{"bc"}}},{'d', {{"ef"}}}}, {}, ""}},
    {"ab-a-bc-d-ef-a-gh", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {}, {{'a', {{"bc"}}},{'d', {{"ef"}}}, {'a', {{"gh"}}}}, {}, ""}},
    {"ab-a-b", {}, false},
    {"ab-a-bcdefghij", {}, false},
    // Private use
    {"ab-x-b-cdefghij", {Bcp47TagType::WELL_FORMED, "ab", {}, "", "", {}, {}, {"b", "cdefghij"}, ""}},
    {"ab-x-bcdefghij", {}, false},
    // Combine all subtags
    {"ab-ext-bcde-fg-abcde-0abc-1def-e-abcd-ef-f-ef-x-a-bcd",
      {Bcp47TagType::WELL_FORMED, "ab", {"ext"}, "bcde", "fg", {{"abcde"}, {"0abc"}, {"1def"}}, {{'e', {{"abcd"}, {"ef"}}},{'f', {{"ef"}}}}, {"a", "bcd"}, ""}},
    // Just a private use subtag
    {"x-a-bcd", {Bcp47TagType::PRIVATE_USE, "", {}, "", "", {}, {}, {"a", "bcd"}, ""}},
    // Irregular grandfathered
    {"i-ami", {Bcp47TagType::GRANDFATHERED, "", {}, "", "", {}, {}, {}, "i-ami"}},
    // Regular grandfathered
    {"cel-gaulish", {Bcp47TagType::GRANDFATHERED, "", {}, "", "", {}, {}, {}, "cel-gaulish"}},
    // empty or whitespace
    {"", {}, false},
    {" ", {}, false},
    {"  ", {}, false},
    // surrounding whitespace
    {" en ", {Bcp47TagType::WELL_FORMED, "en", {}, "", "", {}, {}, {}, ""}},
    // various case
    {"EN", {Bcp47TagType::WELL_FORMED, "en", {}, "", "", {}, {}, {}, ""}},
    {"En", {Bcp47TagType::WELL_FORMED, "en", {}, "", "", {}, {}, {}, ""}},
};
// clang-format on

class Bcp47ParserTester : public testing::Test, public testing::WithParamInterface<TestBcp47Parser>
{
};

TEST_P(Bcp47ParserTester, Parse)
{
  auto& param = GetParam();
  auto actual = CBcp47Parser::Parse(param.input);

  EXPECT_EQ(param.status, actual.has_value());
  if (param.status && actual.has_value())
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, actual.value());
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nBcp47Parser,
                         Bcp47ParserTester,
                         testing::ValuesIn(ParseBcp47Tests));
