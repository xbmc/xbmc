/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47Parser.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

namespace KODI::UTILS::I18N
{
std::ostream& operator<<(std::ostream& os, const Bcp47Extension& obj)
{
  os << obj.name << ": {" << StringUtils::Join(obj.segments, ",") << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const CBcp47& obj)
{
  if (obj.IsGrandfathered()) [[unlikely]]
  {
    os << "BCP47 (grandfathered: " << obj.GetGrandfathered() << ")";
    return os;
  }

  os << "BCP47 (language: " << obj.GetLanguage() << ", extended languages: {"
     << StringUtils::Join(obj.GetExtLangs(), ",") << "}, script: " << obj.GetScript()
     << ", region: " << obj.GetRegion() << ", variants: {"
     << StringUtils::Join(obj.GetVariants(), ",") << "}, extensions: {";

  for (const auto& ext : obj.GetExtensions())
    os << ext << " ";

  os << "}, private use: {" << StringUtils::Join(obj.GetPrivateUse(), ", ") << "}, ";
  os << "grandfathered: " << obj.GetGrandfathered() << ")";

  return os;
}

bool operator==(const ParsedBcp47Tag& test, const CBcp47& actual)
{
  return test.m_language == actual.GetLanguage() && test.m_extLangs == actual.GetExtLangs() &&
         test.m_script == actual.GetScript() && test.m_region == actual.GetRegion() &&
         test.m_variants == actual.GetVariants() && test.m_extensions == actual.GetExtensions() &&
         test.m_privateUse == actual.GetPrivateUse() &&
         test.m_grandfathered == actual.GetGrandfathered();
}

std::ostream& operator<<(std::ostream& os, const ParsedBcp47Tag& obj)
{
  os << "BCP47 (language: " << obj.m_language << ", extended languages: {"
     << StringUtils::Join(obj.m_extLangs, ",") << "}, script: " << obj.m_script
     << ", region: " << obj.m_region << ", variants: {" << StringUtils::Join(obj.m_variants, ",")
     << "}, extensions: {";

  for (const auto& ext : obj.m_extensions)
    os << ext << " ";

  os << "}, private use: {" << StringUtils::Join(obj.m_privateUse, ", ") << "}, ";
  os << "grandfathered: " << obj.m_grandfathered << ")";

  return os;
}
} // namespace KODI::UTILS::I18N

struct TestParseBcp47
{
  std::string input;
  ParsedBcp47Tag expected;
  bool status{true};
};

std::ostream& operator<<(std::ostream& os, const TestParseBcp47& rhs)
{
  return os << rhs.input;
}

const TestParseBcp47 ParseBcp47Tests[] = {
    // clang-format off
    // ISO 639-1 code
    {"ab", {.type = Bcp47TagType::REGULAR, .m_language = "ab"}},
    // ISO 639-2 code
    {"abc", {.type = Bcp47TagType::REGULAR, .m_language = "abc"}},
    // ISO 639-2 code with extended language subtags
    {"abc-def-ghi", {.type = Bcp47TagType::REGULAR, .m_language = "abc", .m_extLangs = {"def", "ghi"}}},
    // registered 5-8 letters code
    {"abcde", {.type = Bcp47TagType::REGULAR, .m_language = "abcde"}},
    // invalid, more than 8 letters
    {"montenegro", {}, false},
    // script
    {"ab-abcd", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "abcd"}},
    // Region ISO 3166-1
    {"ab-ab", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "ab"}},
    // Region UN M.49
    {"ab-012", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "012"}},
    // Variants
    {"ab-abcde-bcdefghi", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {{"abcde"}, {"bcdefghi"}}}},
    {"ab-abcde-0abc-1def", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {{"abcde"}, {"0abc"}, {"1def"}}}},
    // Extensions
    {"ab-a-bcdefghi-jk", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {}, .m_extensions = {{'a', {{"bcdefghi"}, {"jk"}}}}}},
    {"ab-a-bc-de-a-fg-hi", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {}, .m_extensions = {{'a', {{"bc"}, {"de"}}}, {'a', {{"fg"}, {"hi"}}}}}},
    {"ab-a-bc-d-ef", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {}, .m_extensions = {{'a', {{"bc"}}},{'d', {{"ef"}}}}}},
    {"ab-a-bc-d-ef-a-gh", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {}, .m_extensions = {{'a', {{"bc"}}},{'d', {{"ef"}}}, {'a', {{"gh"}}}}}},
    // Private use
    {"ab-x-b-cdefghij", {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {}, .m_privateUse = {"b", "cdefghij"}}},
    // Combine all subtags
    {"ab-ext-bcde-fg-abcde-0abc-1def-e-abcd-ef-f-ef-x-a-bcd",
      {.type = Bcp47TagType::REGULAR, .m_language = "ab", .m_extLangs = {"ext"}, .m_script = "bcde", .m_region = "fg", .m_variants = {{"abcde"}, {"0abc"}, {"1def"}},
      .m_extensions = {{'e', {{"abcd"}, {"ef"}}},{'f', {{"ef"}}}}, .m_privateUse = {"a", "bcd"}}},
    // Just a private use subtag
    {"x-a-bcd", {.type = Bcp47TagType::REGULAR, .m_language = "", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {}, .m_privateUse = {"a", "bcd"}}},
    // Irregular grandfathered
    {"i-ami", {.type = Bcp47TagType::REGULAR, .m_language = "", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {}, .m_privateUse = {}, .m_grandfathered = "i-ami"}},
    // Regular grandfathered
    {"cel-gaulish", {.type = Bcp47TagType::REGULAR, .m_language = "", .m_extLangs = {}, .m_script = "", .m_region = "", .m_variants = {}, .m_privateUse = {}, .m_grandfathered = "cel-gaulish"}},
    // clang-format on
};

class ParseTagTester : public testing::Test, public testing::WithParamInterface<TestParseBcp47>
{
};

TEST_P(ParseTagTester, ParseTag)
{
  auto& param = GetParam();

  auto actual = CBcp47::ParseTag(param.input);

  EXPECT_EQ(param.status, actual.has_value());

  if (param.status && actual.has_value())
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, actual.value());
  }
}

INSTANTIATE_TEST_SUITE_P(TestI18nBcp47Parser, ParseTagTester, testing::ValuesIn(ParseBcp47Tests));
