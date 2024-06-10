/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LangInfo.h"
#include "guilib/GUIControlFactory.h"
#include "guilib/GUIFont.h"
#include "guilib/GUITexture.h"
#include "guilib/LocalizeStrings.h"
#include "utils/XBMCTinyXML.h"

#include <gtest/gtest.h>

#include <limits>
#include <string>

using namespace KODI;

class CGFTestable : public CGUIControlFactory
{
public:
  static std::string GetType(const TiXmlElement* pControlNode)
  {
    return CGUIControlFactory::GetType(pControlNode);
  }

  static bool GetIntRange(const TiXmlNode* pRootNode,
                          const char* strTag,
                          int& iMinValue,
                          int& iMaxValue,
                          int& iIntervalValue)
  {
    return CGUIControlFactory::GetIntRange(pRootNode,
                                           strTag,
                                           iMinValue,
                                           iMaxValue,
                                           iIntervalValue);
  }

  static bool GetFloatRange(const TiXmlNode* pRootNode,
                            const char* strTag,
                            float& fMinValue,
                            float& fMaxValue,
                            float& fIntervalValue)
  {
    return CGUIControlFactory::GetFloatRange(pRootNode,
                                             strTag,
                                             fMinValue,
                                             fMaxValue,
                                             fIntervalValue);
  }

  static bool GetPosition(const TiXmlNode* node,
                          const char* tag,
                          const float parentSize,
                          float& value)
  {
    return CGUIControlFactory::GetPosition(node,
                                           tag,
                                           parentSize,
                                           value);
  }

  static bool GetDimension(const TiXmlNode* node,
                           const char* strTag,
                           const float parentSize,
                           float& value,
                           float& min)
  {
    return CGUIControlFactory::GetDimension(node,
                                            strTag,
                                            parentSize,
                                            value,
                                            min);
  }

  static bool GetDimensions(const TiXmlNode* node,
                            const char* leftTag,
                            const char* rightTag,
                            const char* centerLeftTag,
                            const char* centerRightTag,
                            const char* widthTag,
                            const float parentSize,
                            float& left,
                            float& width,
                            float& min_width)
  {
    return CGUIControlFactory::GetDimensions(node,
                                             leftTag,
                                             rightTag,
                                             centerLeftTag,
                                             centerRightTag,
                                             widthTag,
                                             parentSize,
                                             left,
                                             width,
                                             min_width);
  }

  static bool GetMovingSpeedConfig(const TiXmlNode* pRootNode,
                                   const char* strTag,
                                   UTILS::MOVING_SPEED::MapEventConfig& movingSpeedCfg)
  {
    return CGUIControlFactory::GetMovingSpeedConfig(pRootNode,
                                                    strTag,
                                                    movingSpeedCfg);
  }

  static bool GetConditionalVisibility(const TiXmlNode* control,
                                       std::string& condition,
                                       std::string& allowHiddenFocus)
  {
    return CGUIControlFactory::GetConditionalVisibility(control,
                                                        condition,
                                                        allowHiddenFocus);
  }

  static bool GetString(const TiXmlNode* pRootNode,
                        const char* strTag,
                        std::string& strString)
  {
    return CGUIControlFactory::GetString(pRootNode,
                                         strTag,
                                         strString);
  }
};

namespace {

using namespace std::string_literals;

struct AlignmentTest
{
  std::string def;
  uint32_t align;
  bool result = true;
};

const auto AlignmentTests = std::array{
    AlignmentTest{R"(<root><test>right</test></root>)"s, XBFONT_RIGHT},
    AlignmentTest{R"(<root><test>bottom</test></root>)"s, XBFONT_RIGHT},
    AlignmentTest{R"(<root><test>center</test></root>)"s, XBFONT_CENTER_X},
    AlignmentTest{R"(<root><test>justify</test></root>)"s, XBFONT_JUSTIFIED},
    AlignmentTest{R"(<root><test>left</test></root>)"s, XBFONT_LEFT},
    AlignmentTest{R"(<root><test>foo</test></root>)"s, XBFONT_LEFT},
    AlignmentTest{R"(<root><test/></root>)"s, 0, false},
    AlignmentTest{R"(<root></root>)"s, 0, false},
};

class TestAlignment : public testing::WithParamInterface<AlignmentTest>, public testing::Test
{
};

const auto AlignmentYTests = std::array{
    AlignmentTest{R"(<root><test>center</test></root>)"s, XBFONT_CENTER_Y},
    AlignmentTest{R"(<root><test>bottom</test></root>)"s, 0},
    AlignmentTest{R"(<root><test/></root>)"s,
                  std::numeric_limits<uint32_t>::max(), false},
    AlignmentTest{R"(<root></root>)"s,
                  std::numeric_limits<uint32_t>::max(), false},
};

class TestAlignmentY : public testing::WithParamInterface<AlignmentTest>, public testing::Test
{
};

struct AspectRatioTest
{
  std::string def;
  CAspectRatio ratio;
  bool result = true;
};

const auto AspectRatioTests = std::array{
    AspectRatioTest{R"(<root><test align="center">keep</test></root>)"s,
                    {CAspectRatio::AR_KEEP, ASPECT_ALIGN_CENTER, true}},
    AspectRatioTest{R"(<root><test align="right">scale</test></root>)"s,
                    {CAspectRatio::AR_SCALE, ASPECT_ALIGN_RIGHT, true}},
    AspectRatioTest{R"(<root><test align="left">center</test></root>)"s,
                    {CAspectRatio::AR_CENTER, ASPECT_ALIGN_LEFT, true}},
    AspectRatioTest{R"(<root><test align="center">stretch</test></root>)"s,
                    {CAspectRatio::AR_STRETCH, ASPECT_ALIGN_CENTER, true}},
    AspectRatioTest{R"(<root><test aligny="center" scalediffuse="true">keep</test></root>)"s,
                    {CAspectRatio::AR_KEEP, ASPECT_ALIGNY_CENTER, true}},
    AspectRatioTest{R"(<root><test aligny="bottom" scalediffuse="yes">keep</test></root>)"s,
                    {CAspectRatio::AR_KEEP, ASPECT_ALIGNY_BOTTOM, true}},
    AspectRatioTest{R"(<root><test align="right" aligny="top" scalediffuse="no">keep</test></root>)"s,
                    {CAspectRatio::AR_KEEP, ASPECT_ALIGN_RIGHT | ASPECT_ALIGNY_TOP, false}},
    AspectRatioTest{R"(<root><test scalediffuse="false">keep</test></root>)"s,
                    {CAspectRatio::AR_KEEP, ASPECT_ALIGN_CENTER, false}},
    AspectRatioTest{R"(<root><test/></root>)"s,
                    {CAspectRatio::AR_STRETCH, ASPECT_ALIGN_CENTER, true}, false},
    AspectRatioTest{R"(<root/>)"s,
                    {CAspectRatio::AR_STRETCH, ASPECT_ALIGN_CENTER, true}, false},
};

class TestAspectRatio : public testing::WithParamInterface<AspectRatioTest>, public testing::Test
{
};

struct ConditionalVisibilityTest
{
  std::string def;
  std::string condition;
  std::string hidden;
  bool result = true;
};

class TestConditionalVisibility : public testing::WithParamInterface<ConditionalVisibilityTest>
                                , public testing::Test
{
};

const auto ConditionalVisibilityTests = std::array{
    ConditionalVisibilityTest{R"(<root>
                                  <visible>foo</visible>
                                 </root>)"s,
                              "foo"s, ""s},
    ConditionalVisibilityTest{R"(<root>
                                  <visible allowhiddenfocus="bar">foo</visible>
                                 </root>)"s,
                              "foo"s, "bar"s},
    ConditionalVisibilityTest{R"(<root>
                                  <visible allowhiddenfocus="bar">foo</visible>
                                  <visible allowhiddenfocus="foobar">bar</visible>
                                 </root>)"s,
                              "[foo] + [bar]"s, "foobar"s},
    ConditionalVisibilityTest{R"(<root/>)", ""s, ""s, false},
};


struct DimensionTest
{
  std::string def;
  float min, value;
  bool result = true;
};

class TestGetDimension : public testing::WithParamInterface<DimensionTest>, public testing::Test
{
};

const auto DimensionTests = std::array{
    DimensionTest{R"(<root><test>0.1</test></root>)"s,
                  0.f, 0.1f},
    DimensionTest{R"(<root><test max="3.0">auto</test></root>)"s,
                  1.f, 3.f},
    DimensionTest{R"(<root><test max="3.0r">auto</test></root>)"s,
                  1.f, -2.9f},
    DimensionTest{R"(<root><test max="3.0%">auto</test></root>)"s,
                  1.f, 3.f * 0.1f / 100.f},
    DimensionTest{R"(<root><test min="2.0" max="3.0">auto</test></root>)"s,
                  2.f, 3.f},
    DimensionTest{R"(<root><test min="2.0r" max="3.0">auto</test></root>)"s,
                  -1.9f, 3.f},
    DimensionTest{R"(<root><test min="2.0%" max="3.0">auto</test></root>)"s,
                  2.f * 0.1f / 100.f, 3.f},
    DimensionTest{R"(<root><test>something</test></root>)"s,
                  0.f, 0.f},
    DimensionTest{R"(<root><test/></root>)"s,
                  0.f, 0.f, false},
    DimensionTest{R"(<root/>)"s,
                  0.f, 0.f, false},
};

struct DimensionsTest
{
  std::string def;
  float left, width, min_width;
  bool result = true;
};

class TestGetDimensions : public testing::WithParamInterface<DimensionsTest>, public testing::Test
{
};

const auto DimensionsTests =  std::array{
    DimensionsTest{R"(<root><left>0.1</left><width>0.2</width></root>)"s,
                   0.1f, 0.2f, 0.f},
    DimensionsTest{R"(<root><left>0.01</left><right>50%</right></root>)"s,
                   0.01f, 0.1f - 0.1f * 50.f / 100.f - 0.01f, 0.f},
    DimensionsTest{R"(<root><center_left>0.025f</center_left><right>50%</right></root>)"s,
                   0.f, (0.05f - 0.025f) * 2, 0.f},
    DimensionsTest{R"(<root><center_right>0.025f</center_right><width min="0.5" max="0.2">auto</width></root>)"s,
                   (0.1f - 0.025f) - 0.2f / 2, 0.2f, 0.5f},
};

template<class T>
struct RangeTest
{
  std::string def;
  T min, max, interval;
  bool result = true;
};

class TestGetFloatRange : public testing::WithParamInterface<RangeTest<float>>, public testing::Test
{
};

const auto FloatRangeTests = std::array{
    RangeTest<float>{R"(<root><test>1.0,100.0,0.25</test></root>)"s,
                     1.f, 100.f, 0.25f},
    RangeTest<float>{R"(<root><test>1.0,100.0</test></root>)"s,
                     1.f, 100.f, 0.f},
    RangeTest<float>{R"(<root><test>,100.0,5.0</test></root>)"s,
                     0.f, 100.f, 5.f},
    RangeTest<float>{R"(<root><test>1.0</test></root>)"s,
                     1.f, 0.f, 0.f},
    RangeTest<float>{R"(<root><test/></root>)"s,
                     0.f, 0.f, 0.f, false},
    RangeTest<float>{R"(<root/>)"s,
                     0.f, 0.f, 0.f, false},
};

struct HitRectTest
{
  std::string def;
  CRect rect;
  bool result = true;
};

class TestGetHitRect : public testing::WithParamInterface<HitRectTest>, public testing::Test
{
};

const auto HitRectTests = std::array{
    HitRectTest{R"(<root><hitrect x="1.0" y="2.0" w="3.0" h="4.0"/></root>)"s,
                CRect{1.0, 2.0, 4.0, 6.0}},
    HitRectTest{R"(<root><hitrect y="2.0" right="3.0" h="4.0"/></root>)"s,
                CRect{0.0, 2.0, 0.0, 6.0}},
    HitRectTest{R"(<root><hitrect x="1.0" y="2.0" w="3.0" bottom="4.0"/></root>)"s,
                CRect{1.0, 2.0, 4.0, 2.0}},
    HitRectTest{R"(<root/>)"s,
                CRect{0.0, 0.0, 0.0, 0.0}, false},
};

class TestGetIntRange : public testing::WithParamInterface<RangeTest<int>>, public testing::Test
{
};

const auto IntRangeTests = std::array{
    RangeTest<int>{R"(<root><test>1,100,5</test></root>)"s,
                   1, 100, 5},
    RangeTest<int>{R"(<root><test>1,100</test></root>)"s,
                   1, 100, 0},
    RangeTest<int>{R"(<root><test>,100,5</test></root>)"s,
                   0, 100, 5},
    RangeTest<int>{R"(<root><test>1</test></root>)"s,
                   1, 0, 0},
    RangeTest<int>{R"(<root><test/></root>)"s,
                   0, 0, 0, false},
    RangeTest<int>{R"(<root/>)"s,
                   0, 0, 0, false},
};

struct MovingSpeedTest
{
  std::string def;
  UTILS::MOVING_SPEED::MapEventConfig config;
  bool result = true;
};

class TestGetMovingSpeed : public testing::WithParamInterface<MovingSpeedTest>, public testing::Test
{
};

const auto MovingSpeedTests = std::array{
    MovingSpeedTest{R"(<root><test acceleration="1.0" maxvelocity="2.0"
                                   resettimeout="3" delta="4.0">
                         <eventconfig type="up"/>
                         <eventconfig type="down" acceleration="2.0"/>
                         <eventconfig type="left" maxvelocity="3.0"/>
                         <eventconfig type="right" resettimeout="4"/>
                         <eventconfig type="none" delta="5"/>
                       </test></root>)"s,
                    {{UTILS::MOVING_SPEED::EventType::UP,
                      UTILS::MOVING_SPEED::EventCfg{1.0, 2.0, 3, 4.0}},
                     {UTILS::MOVING_SPEED::EventType::DOWN,
                      UTILS::MOVING_SPEED::EventCfg{2.0, 2.0, 3, 4.0}},
                     {UTILS::MOVING_SPEED::EventType::LEFT,
                      UTILS::MOVING_SPEED::EventCfg{1.0, 3.0, 3, 4.0}},
                     {UTILS::MOVING_SPEED::EventType::RIGHT,
                      UTILS::MOVING_SPEED::EventCfg{1.0, 2.0, 4, 4.0}},
                     {UTILS::MOVING_SPEED::EventType::NONE,
                      UTILS::MOVING_SPEED::EventCfg{1.0, 2.0, 3, 5.0}},
                    }},
    MovingSpeedTest{R"(<root><test>
                         <eventconfig type="up"/>
                         <eventconfig type="down" acceleration="2.0"/>
                         <eventconfig type="left" maxvelocity="3.0"/>
                         <eventconfig type="right" resettimeout="4"/>
                         <eventconfig type="none" delta="5"/>
                       </test></root>)"s,
                    {{UTILS::MOVING_SPEED::EventType::UP,
                      UTILS::MOVING_SPEED::EventCfg{0.0, 0.0, 0, 0.0}},
                     {UTILS::MOVING_SPEED::EventType::DOWN,
                      UTILS::MOVING_SPEED::EventCfg{2.0, 0.0, 0, 0.0}},
                     {UTILS::MOVING_SPEED::EventType::LEFT,
                      UTILS::MOVING_SPEED::EventCfg{0.0, 3.0, 0, 0.0}},
                     {UTILS::MOVING_SPEED::EventType::RIGHT,
                      UTILS::MOVING_SPEED::EventCfg{0.0, 0.0, 4, 0.0}},
                     {UTILS::MOVING_SPEED::EventType::NONE,
                      UTILS::MOVING_SPEED::EventCfg{0.0, 0.0, 0, 5.0}},
                    }},
    MovingSpeedTest{R"(<root><test/></root>)"s,
                    {}, true},
    MovingSpeedTest{R"(<root/>)"s,
                    {}, false},
};

struct PositionTest
{
  std::string def;
  float pos;
  bool result = true;
};

class TestGetPosition : public testing::WithParamInterface<PositionTest>, public testing::Test
{
};

const auto PositionTests = std::array{
    PositionTest{R"(<root><test>0.1</test></root>)"s, 0.1f},
    PositionTest{R"(<root><test>0.2r</test></root>)"s, -0.1f},
    PositionTest{R"(<root><test>0.2%</test></root>)"s,
                 0.2f * 0.1f / 100.f},
    PositionTest{R"(<root><test>something</test></root>)"s, 0.f},
    PositionTest{R"(<root><test/></root>)"s, 0.f, false},
    PositionTest{R"(<root/>)"s, 0.f, false},
};

struct StringTest
{
  std::string def;
  std::string value;
  bool result = true;
};

class TestGetString : public testing::WithParamInterface<StringTest>, public testing::Test
{
};

const auto StringTests = std::array{
    StringTest{R"(<root><test>foo</test></root>)"s, "foo"s},
    StringTest{R"(<root><test>1</test></root>)"s, "Pictures"s},
    StringTest{R"(<root><test>-1</test></root>)"s, "-1"s},
    StringTest{R"(<root><test/></root>)"s, ""s, true},
    StringTest{R"(<root/>)"s, ""s, false},
};

struct TypeTest
{
  std::string def;
  std::string type;
};

class TestGetType : public testing::WithParamInterface<TypeTest>, public testing::Test
{
};

const auto TypeTests = std::array{
    TypeTest{R"(<root type="foo"/></root>)"s, "foo"s},
    TypeTest{R"(<root><type>foo</type></root>)"s, "foo"s},
    TypeTest{R"(<root/>)"s, ""s},
};

}

TEST_P(TestAlignment, GetAlignment)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  uint32_t align{};
  EXPECT_EQ(CGFTestable::GetAlignment(doc.RootElement(),
                                      "test", align),
                                      GetParam().result);
  EXPECT_EQ(align, GetParam().align);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestAlignment,
                         testing::ValuesIn(AlignmentTests));

TEST_P(TestAlignmentY, GetAlignmentY)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  uint32_t align = std::numeric_limits<uint32_t>::max();
  EXPECT_EQ(CGFTestable::GetAlignmentY(doc.RootElement(),
                                       "test", align),
                                      GetParam().result);
  EXPECT_EQ(align, GetParam().align);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestAlignmentY,
                         testing::ValuesIn(AlignmentYTests));

TEST_P(TestAspectRatio, GetAspectRatio)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  CAspectRatio ratio;
  EXPECT_EQ(CGFTestable::GetAspectRatio(doc.RootElement(),
                                        "test", ratio), GetParam().result);
  EXPECT_EQ(ratio.ratio, GetParam().ratio.ratio);
  EXPECT_EQ(ratio.align, GetParam().ratio.align);
  EXPECT_EQ(ratio.scaleDiffuse, GetParam().ratio.scaleDiffuse);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestAspectRatio,
                         testing::ValuesIn(AspectRatioTests));

TEST_P(TestGetDimension, GetDimension)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  float min = 0.f, value = 0.f;
  EXPECT_EQ(CGFTestable::GetDimension(doc.RootElement(),
                                     "test", 0.1f, value, min),
                                     GetParam().result);
  EXPECT_EQ(min, GetParam().min);
  EXPECT_EQ(value, GetParam().value);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetDimension,
                         testing::ValuesIn(DimensionTests));

TEST_P(TestGetDimensions, GetDimensions)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  float left = 0.f, width = 0.f, min_width = 0.f;
  EXPECT_EQ(CGFTestable::GetDimensions(doc.RootElement(),
                                       "left",
                                       "right",
                                       "center_left",
                                       "center_right",
                                       "width", 0.1f,
                                       left, width, min_width),
                                       GetParam().result);
  EXPECT_EQ(left, GetParam().left);
  EXPECT_EQ(width, GetParam().width);
  EXPECT_EQ(min_width, GetParam().min_width);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetDimensions,
                         testing::ValuesIn(DimensionsTests));

TEST_P(TestGetFloatRange, GetFloatRange)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  float min = 0.f, max = 0.f, interval = 0.f;
  EXPECT_EQ(CGFTestable::GetFloatRange(doc.RootElement(),
                                       "test", min, max, interval),
                                       GetParam().result);
  EXPECT_EQ(min, GetParam().min);
  EXPECT_EQ(max, GetParam().max);
  EXPECT_EQ(interval, GetParam().interval);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetFloatRange,
                         testing::ValuesIn(FloatRangeTests));

TEST_P(TestGetHitRect, GetHitRect)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  CRect rect, parentRect{1.0, 2.0, 3.0, 4.0};
  EXPECT_EQ(CGFTestable::GetHitRect(doc.RootElement(),
                                    rect, parentRect),
                                    GetParam().result);
  EXPECT_EQ(rect.x1, GetParam().rect.x1);
  EXPECT_EQ(rect.x2, GetParam().rect.x2);
  EXPECT_EQ(rect.y1, GetParam().rect.y1);
  EXPECT_EQ(rect.y2, GetParam().rect.y2);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetHitRect,
                         testing::ValuesIn(HitRectTests));

TEST_P(TestGetIntRange, GetIntRange)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  int min = 0, max = 0, interval = 0;
  EXPECT_EQ(CGFTestable::GetIntRange(doc.RootElement(),
                                     "test", min, max, interval),
                                     GetParam().result);
  EXPECT_EQ(min, GetParam().min);
  EXPECT_EQ(max, GetParam().max);
  EXPECT_EQ(interval, GetParam().interval);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetIntRange,
                         testing::ValuesIn(IntRangeTests));

TEST_P(TestGetMovingSpeed, GetMovingSpeed)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  UTILS::MOVING_SPEED::MapEventConfig config;
  EXPECT_EQ(CGFTestable::GetMovingSpeedConfig(doc.RootElement(),
                                              "test", config),
                                              GetParam().result);
  EXPECT_EQ(config, GetParam().config);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetMovingSpeed,
                         testing::ValuesIn(MovingSpeedTests));

TEST_P(TestGetPosition, GetPosition)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  float pos = 0.f;
  EXPECT_EQ(CGFTestable::GetPosition(doc.RootElement(),
                                     "test", 0.1f, pos),
                                     GetParam().result);
  EXPECT_EQ(pos, GetParam().pos);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetPosition,
                         testing::ValuesIn(PositionTests));

TEST_P(TestGetString, GetString)
{
  ASSERT_TRUE(g_localizeStrings.Load(g_langInfo.GetLanguagePath(),
                                     "resource.language.en_gb"));
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  std::string value;
  EXPECT_EQ(CGFTestable::GetString(doc.RootElement(),
                                   "test", value), GetParam().result);
  EXPECT_EQ(value, GetParam().value);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetString,
                         testing::ValuesIn(StringTests));

TEST_P(TestGetType, GetType)
{
  CXBMCTinyXML doc;
  doc.Parse(GetParam().def);
  std::string type;
  EXPECT_EQ(CGFTestable::GetType(doc.RootElement()), GetParam().type);
}

INSTANTIATE_TEST_SUITE_P(TestGUIControlFactory,
                         TestGetType,
                         testing::ValuesIn(TypeTests));
