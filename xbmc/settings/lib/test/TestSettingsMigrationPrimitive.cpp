/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/SettingsMigrationPrimitive.h"
#include "utils/XMLUtils.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace KODI::SETTINGS;

namespace
{
// All tests will attempt this conversion
constexpr std::string_view OLDSETTINGID = "a.b";
constexpr std::string_view NEWSETTINGID = "c.d";

// Testing of impl::ConvertSingleSetting<>(): use mock setting types to exercise the various code
// paths and output codes

// Base setting mock type to avoid redefining the pure virtual functions multiple times
template<typename T>
class MockSettingBase : public CTraitedSetting<T, SettingType::Unknown>
{
public:
  using Value = T;

  explicit MockSettingBase(std::string_view id, CSettingsManager* mgr = nullptr)
    : CTraitedSetting<T, SettingType::Unknown>(id, mgr)
  {
  }

  // stubs for CSetting pure virtual functions not relevant to the tests
  std::shared_ptr<CSetting> Clone(std::string_view) const override { return nullptr; }
  void MergeDetails(const CSetting&) override {}
  std::string ToString() const override { return {}; }
  bool Equals(const std::string&) const override { return false; }
  bool CheckValidity(const std::string&) const override { return true; }
  void Reset() override {}

  // functions used in mocking
  const T& GetValue() const { return m_value; }
  bool SetValue(const T& v)
  {
    m_value = v;
    CSetting::m_changed = m_value != m_default;
    return true;
  }
  void SetDefault(const T& v)
  {
    m_default = v;
    if (!CSetting::m_changed)
      m_value = m_default;
  }

protected:
  T m_value{};
  T m_default{};
};

// Concrete mock types
class MockSettingAlwaysFail : public MockSettingBase<int>
{
public:
  using MockSettingBase<int>::MockSettingBase;
  bool FromString(const std::string&) override { return false; }
};

class MockSettingAlwaysSuccess : public MockSettingBase<int>
{
public:
  using MockSettingBase<int>::MockSettingBase;

  bool FromString(const std::string&) override
  {
    m_value = 42;
    return true;
  }
  std::string ToString() const override { return std::to_string(m_value); }
};

class MockSettingThrowOnCtor : public MockSettingBase<int>
{
public:
  explicit MockSettingThrowOnCtor(std::string_view id, CSettingsManager* mgr = nullptr)
    : MockSettingBase<int>(id, mgr)
  {
    throw std::bad_alloc{};
  }
  bool FromString(const std::string&) override { return true; }
};

// A few helpers
static std::string Serialize(const CXBMCTinyXML& doc)
{
  return XMLUtils::NodeStringSerialization(doc.RootElement(),
                                           XMLUtils::SerializationFormat::COMPACT);
}

// Used wherever the converter output is irrelevant
static std::pair<int, int> TrivialConverter(int)
{
  return {1, 0};
}

// Parametrized test parameters shared by a few suites
struct ConvertTest
{
  std::string m_xml;
  std::string_view m_expectedXml;
};

} // namespace

// ---------------------------------------------------------------------------
// NOT_PRESENT tests
// ---------------------------------------------------------------------------

class TestConvertSingleSettingNotPresent : public ::testing::Test,
                                           public testing::WithParamInterface<ConvertTest>
{
};

ConvertTest NotPresentTests[] = {
    // V2 format: empty document
    {R"(<settings version="2" />)", R"(<settings version="2" />)"},
    {R"(<settings version="2"></settings>)", R"(<settings version="2" />)"},
    // V2 format: a different setting is present
    {R"(<settings version="2"><setting id="foo.bar">1</setting></settings>)",
     R"(<settings version="2"><setting id="foo.bar">1</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.bar">1</setting></settings>)",
     R"(<settings version="2"><setting id="a.bar">1</setting></settings>)"},
    {R"(<settings version="2"><setting id="foo.b">1</setting></settings>)",
     R"(<settings version="2"><setting id="foo.b">1</setting></settings>)"},
    // V1 format: empty document
    {R"(<settings />)", R"(<settings />)"},
    {R"(<settings></settings>)", R"(<settings />)"},
    // V1 format: wrong category tag
    {R"(<settings><foo><bar>1</bar></foo></settings>)",
     R"(<settings><foo><bar>1</bar></foo></settings>)"},
    {R"(<settings><a><bar>1</bar></a></settings>)", R"(<settings><a><bar>1</bar></a></settings>)"},
    {R"(<settings><foo><b>1</b></foo></settings>)", R"(<settings><foo><b>1</b></foo></settings>)"},
};

TEST_P(TestConvertSingleSettingNotPresent, ReturnsNotPresentAndDocumentIsUnchanged)
{
  const auto& p = GetParam();
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(p.m_xml));

  const auto result =
      impl::ConvertSingleSetting<MockSettingAlwaysSuccess, MockSettingAlwaysSuccess>(
          doc.RootElement(), OLDSETTINGID, NEWSETTINGID, TrivialConverter);

  EXPECT_EQ(SettingConversionResult::NOT_PRESENT, result);
  EXPECT_EQ(p.m_expectedXml, Serialize(doc));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationHelper,
                         TestConvertSingleSettingNotPresent,
                         testing::ValuesIn(NotPresentTests));

// ---------------------------------------------------------------------------
// FAILURE tests. Testable with throwing constructors, TinyXML errors are not practical
// ---------------------------------------------------------------------------

struct FailureCtorThrowTest
{
  std::string m_xml;
};

class TestConvertSingleSettingFailureCtorThrow
  : public ::testing::Test,
    public testing::WithParamInterface<FailureCtorThrowTest>
{
};

FailureCtorThrowTest FailureCtorThrowTests[] = {
    // V2 format
    {R"(<settings version="2"><setting id="a.b">x</setting></settings>)"},
    // V1 format
    {R"(<settings><a><b>x</b></a></settings>)"},
};

TEST_P(TestConvertSingleSettingFailureCtorThrow, ReturnsFailureWhenFromTCtorThrows)
{
  const auto& p = GetParam();
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(p.m_xml));

  const auto result = impl::ConvertSingleSetting<MockSettingThrowOnCtor, MockSettingAlwaysSuccess>(
      doc.RootElement(), OLDSETTINGID, NEWSETTINGID, TrivialConverter);

  EXPECT_EQ(SettingConversionResult::FAILURE, result);
}

TEST_P(TestConvertSingleSettingFailureCtorThrow, ReturnsFailureWhenToTCtorThrows)
{
  const auto& p = GetParam();
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(p.m_xml));

  const auto result = impl::ConvertSingleSetting<MockSettingAlwaysSuccess, MockSettingThrowOnCtor>(
      doc.RootElement(), OLDSETTINGID, NEWSETTINGID, TrivialConverter);

  EXPECT_EQ(SettingConversionResult::FAILURE, result);
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationHelper,
                         TestConvertSingleSettingFailureCtorThrow,
                         testing::ValuesIn(FailureCtorThrowTests));

// ---------------------------------------------------------------------------
// INVALID tests
// ---------------------------------------------------------------------------

class TestConvertSingleSettingInvalid : public ::testing::Test,
                                        public testing::WithParamInterface<ConvertTest>
{
};

ConvertTest InvalidTests[] = {
    // V2 format: empty text
    {R"(<settings version="2"><setting id="a.b" /></settings>)",
     R"(<settings version="2"><setting id="a.b" /></settings>)"},
    {R"(<settings version="2"><setting id="a.b"></setting></settings>)",
     R"(<settings version="2"><setting id="a.b" /></settings>)"},
    // V2 format: non-empty text - rejected unconditionally)
    {R"(<settings version="2"><setting id="a.b">bad</setting></settings>)",
     R"(<settings version="2"><setting id="a.b">bad</setting></settings>)"},
    // V1 format: empty text
    {R"(<settings><a><b /></a></settings>)", R"(<settings><a><b /></a></settings>)"},
    {R"(<settings><a><b></b></a></settings>)", R"(<settings><a><b /></a></settings>)"},
    // V1 format: non-empty text
    {R"(<settings><a><b>bad</b></a></settings>)", R"(<settings><a><b>bad</b></a></settings>)"},
};

TEST_P(TestConvertSingleSettingInvalid, ReturnsInvalidAndDocumentIsUnchanged)
{
  const auto& p = GetParam();
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(p.m_xml));

  const auto result = impl::ConvertSingleSetting<MockSettingAlwaysFail, MockSettingAlwaysSuccess>(
      doc.RootElement(), OLDSETTINGID, NEWSETTINGID, TrivialConverter);

  EXPECT_EQ(SettingConversionResult::INVALID, result);
  EXPECT_EQ(p.m_expectedXml, Serialize(doc));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationHelper,
                         TestConvertSingleSettingInvalid,
                         testing::ValuesIn(InvalidTests));

// ---------------------------------------------------------------------------
// CONVERTED tests - output document structural outcomes.
// ---------------------------------------------------------------------------

class TestConvertSingleSettingConvertedStructural : public ::testing::Test,
                                                    public testing::WithParamInterface<ConvertTest>
{
};

ConvertTest ConvertedStructuralTests[] = {
    // V2 format: only the old setting present
    {R"(<settings version="2"><setting id="a.b">anything</setting></settings>)",
     R"(<settings version="2"><setting id="c.d">1</setting></settings>)"},
    // V2 format: unrelated sibling must survive; new node appended after it
    {R"(<settings version="2"><setting id="a.b">x</setting><setting id="foo.bar">y</setting></settings>)",
     R"(<settings version="2"><setting id="foo.bar">y</setting><setting id="c.d">1</setting></settings>)"},
    // V1 format: old node removed, new node in V2 format
    {R"(<settings><a><b>anything</b></a></settings>)",
     R"(<settings><a /><setting id="c.d">1</setting></settings>)"},
    // V1 format: unrelated sibling must survive
    {R"(<settings><a><b>x</b></a><foo><bar>y</bar></foo></settings>)",
     R"(<settings><a /><foo><bar>y</bar></foo><setting id="c.d">1</setting></settings>)"},
    {R"(<settings><a><b>x</b><bar>y</bar></a></settings>)",
     R"(<settings><a><bar>y</bar></a><setting id="c.d">1</setting></settings>)"},
};

TEST_P(TestConvertSingleSettingConvertedStructural, ReturnsConvertedAndDocumentIsCorrect)
{
  const auto& p = GetParam();
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(p.m_xml));

  bool converterCalled = false;
  const auto result =
      impl::ConvertSingleSetting<MockSettingAlwaysSuccess, MockSettingAlwaysSuccess>(
          doc.RootElement(), OLDSETTINGID, NEWSETTINGID,
          [&converterCalled](int) -> std::pair<int, int>
          {
            converterCalled = true;
            return {1, 0}; // newValue != defaultValue for no default="true" in output
          });

  EXPECT_EQ(SettingConversionResult::CONVERTED, result);
  EXPECT_TRUE(converterCalled);
  EXPECT_EQ(p.m_expectedXml, Serialize(doc));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationHelper,
                         TestConvertSingleSettingConvertedStructural,
                         testing::ValuesIn(ConvertedStructuralTests));

// ---------------------------------------------------------------------------
// CONVERTED tests - check that the converter receives the value decoded by FromString()
// ---------------------------------------------------------------------------

TEST(TestSettingsMigrationHelper, ConvertSingleSettingConverterInput)
{
  const std::string xml =
      R"(<settings version="2"><setting id="a.b">anything</setting></settings>)";
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(xml));

  // value 42 hardcoded in MockSettingAlwaysSuccess
  int receivedByConverter = -1;
  const auto result =
      impl::ConvertSingleSetting<MockSettingAlwaysSuccess, MockSettingAlwaysSuccess>(
          doc.RootElement(), OLDSETTINGID, NEWSETTINGID,
          [&receivedByConverter](int v) -> std::pair<int, int>
          {
            receivedByConverter = v;
            return {v, 0};
          });

  EXPECT_EQ(SettingConversionResult::CONVERTED, result);
  EXPECT_EQ(42, receivedByConverter);
}

// ---------------------------------------------------------------------------
// CONVERTED tests - check that the converter output reaches the xml serialization
// ---------------------------------------------------------------------------

struct ConverterForwardingTest
{
  int m_newValue;
  int m_defaultValue;
  std::string_view m_expectedXml;
};

class TestConvertSingleSettingConverterForwarding
  : public ::testing::Test,
    public testing::WithParamInterface<ConverterForwardingTest>
{
public:
  const std::string m_originalXml =
      R"(<settings version="2"><setting id="a.b">anything</setting></settings>)";
};

ConverterForwardingTest ConverterForwardingTests[] = {
    {7, 7, R"(<settings version="2"><setting id="c.d" default="true">7</setting></settings>)"},
    {7, 0, R"(<settings version="2"><setting id="c.d">7</setting></settings>)"},
    {0, 0, R"(<settings version="2"><setting id="c.d" default="true">0</setting></settings>)"},
};

TEST_P(TestConvertSingleSettingConverterForwarding, ConverterOutputIsForwardedToSerializer)
{
  const auto& p = GetParam();
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(m_originalXml));

  const auto result =
      impl::ConvertSingleSetting<MockSettingAlwaysSuccess, MockSettingAlwaysSuccess>(
          doc.RootElement(), OLDSETTINGID, NEWSETTINGID,
          [&p](int) -> std::pair<int, int> { return {p.m_newValue, p.m_defaultValue}; });

  ASSERT_EQ(SettingConversionResult::CONVERTED, result);
  EXPECT_EQ(p.m_expectedXml, Serialize(doc));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationHelper,
                         TestConvertSingleSettingConverterForwarding,
                         testing::ValuesIn(ConverterForwardingTests));

// ---------------------------------------------------------------------------
// ConvertSettingBoolToInt tests - structural
// ---------------------------------------------------------------------------

struct ConvertSettingBoolToIntTest
{
  std::string m_originalSettings;
  SettingConversionResult m_result;
  std::string_view m_serializedOutput;
};

class TestConvertSettingBoolToInt : public ::testing::Test,
                                    public testing::WithParamInterface<ConvertSettingBoolToIntTest>
{
public:
  // Mapping shared by all tests
  const SettingBoolToIntMapping m_mapping{.m_default = 1, .m_false = 1, .m_true = 2};
};

ConvertSettingBoolToIntTest ConvertSettingBoolToIntTests[] = {
    // Successful conversions
    // V2 format
    // Convert a true value
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     SettingConversionResult::CONVERTED,
     R"(<settings version="2"><setting id="c.d">2</setting></settings>)"},
    // Convert a false value
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     SettingConversionResult::CONVERTED,
     R"(<settings version="2"><setting id="c.d" default="true">1</setting></settings>)"},
    // V1 format
    // Convert a true value
    {R"(<settings><a><b>true</b></a></settings>)", SettingConversionResult::CONVERTED,
     R"(<settings><a /><setting id="c.d">2</setting></settings>)"},
    // Convert a false value
    {R"(<settings><a><b>false</b></a></settings>)", SettingConversionResult::CONVERTED,
     R"(<settings><a /><setting id="c.d" default="true">1</setting></settings>)"},

    // Invalid old setting values
    // V2 format
    {R"(<settings version="2"><setting id="a.b">notabool</setting></settings>)",
     SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b">notabool</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b"> true</setting></settings>)",
     SettingConversionResult::INVALID,
     R"(<settings version="2"><setting id="a.b"> true</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b"> </setting></settings>)",
     SettingConversionResult::INVALID, R"(<settings version="2"><setting id="a.b" /></settings>)"},
    // V1 format
    {R"(<settings><a><b>notabool</b></a></settings>)", SettingConversionResult::INVALID,
     R"(<settings><a><b>notabool</b></a></settings>)"},
    {R"(<settings><a><b> true</b></a></settings>)", SettingConversionResult::INVALID,
     R"(<settings><a><b> true</b></a></settings>)"},
    {R"(<settings><a><b></b></a></settings>)", SettingConversionResult::INVALID,
     R"(<settings><a><b /></a></settings>)"},
    {R"(<settings><a><b /></a></settings>)", SettingConversionResult::INVALID,
     R"(<settings><a><b /></a></settings>)"},

    // Old setting is not present - thoroughly tested for ConvertSingleSetting() already
    // V2 format
    {R"(<settings version="2"><setting id="foo.bar">true</setting></settings>)",
     SettingConversionResult::NOT_PRESENT,
     R"(<settings version="2"><setting id="foo.bar">true</setting></settings>)"},
    // V1 format
    {R"(<settings><foo><bar>true</bar></foo></settings>)", SettingConversionResult::NOT_PRESENT,
     R"(<settings><foo><bar>true</bar></foo></settings>)"},
};

TEST_P(TestConvertSettingBoolToInt, Convert)
{
  const auto& params = GetParam();

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(params.m_originalSettings));

  auto conversionResult =
      ConvertSettingBoolToInt(doc.RootElement(), OLDSETTINGID, NEWSETTINGID, m_mapping);
  ASSERT_EQ(params.m_result, conversionResult);

  EXPECT_EQ(
      params.m_serializedOutput,
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationPrimitive,
                         TestConvertSettingBoolToInt,
                         testing::ValuesIn(ConvertSettingBoolToIntTests));

struct ConvertSettingBoolToIntMappingTest
{
  std::string m_originalSettings;
  SettingBoolToIntMapping m_mapping;
  std::string_view m_serializedOutput;
};

class TestConvertSettingBoolToIntMapping
  : public ::testing::Test,
    public testing::WithParamInterface<ConvertSettingBoolToIntMappingTest>
{
};

ConvertSettingBoolToIntMappingTest ConvertSettingBoolToIntMappingTests[] = {
    // True value
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     {.m_default = 1, .m_false = 1, .m_true = 2},
     R"(<settings version="2"><setting id="c.d">2</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     {.m_default = 2, .m_false = 1, .m_true = 2},
     R"(<settings version="2"><setting id="c.d" default="true">2</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     {.m_default = 1, .m_false = 2, .m_true = 2},
     R"(<settings version="2"><setting id="c.d">2</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">true</setting></settings>)",
     {.m_default = 2, .m_false = 2, .m_true = 2},
     R"(<settings version="2"><setting id="c.d" default="true">2</setting></settings>)"},

    // False value
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     {.m_default = 1, .m_false = 1, .m_true = 2},
     R"(<settings version="2"><setting id="c.d" default="true">1</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     {.m_default = 2, .m_false = 1, .m_true = 2},
     R"(<settings version="2"><setting id="c.d">1</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     {.m_default = 2, .m_false = 1, .m_true = 1},
     R"(<settings version="2"><setting id="c.d">1</setting></settings>)"},
    {R"(<settings version="2"><setting id="a.b">false</setting></settings>)",
     {.m_default = 1, .m_false = 1, .m_true = 1},
     R"(<settings version="2"><setting id="c.d" default="true">1</setting></settings>)"},
};

TEST_P(TestConvertSettingBoolToIntMapping, Convert)
{
  const auto& params = GetParam();

  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(params.m_originalSettings));

  auto conversionResult =
      ConvertSettingBoolToInt(doc.RootElement(), OLDSETTINGID, NEWSETTINGID, params.m_mapping);
  ASSERT_EQ(SettingConversionResult::CONVERTED, conversionResult);

  EXPECT_EQ(
      params.m_serializedOutput,
      XMLUtils::NodeStringSerialization(doc.RootElement(), XMLUtils::SerializationFormat::COMPACT));
}

INSTANTIATE_TEST_SUITE_P(TestSettingsMigrationPrimitive,
                         TestConvertSettingBoolToIntMapping,
                         testing::ValuesIn(ConvertSettingBoolToIntMappingTests));
