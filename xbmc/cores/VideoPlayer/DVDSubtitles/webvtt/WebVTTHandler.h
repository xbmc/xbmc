/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIColorManager.h"
#include "utils/ColorUtils.h"
#include "utils/RegExp.h"

#include <deque>
#include <map>
#include <memory>
#include <stdio.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

enum class WebvttSection
{
  UNDEFINED = 0,
  STYLE,
  STYLE_CONTENT,
  REGION,
  CUE,
  CUE_TEXT,
  CUE_METADATA,
  NOTE
};

enum class WebvttAlign
{
  AUTO = 0,
  LEFT,
  CENTER,
  RIGHT,
  START,
  END
};

enum class WebvttVAlign
{
  HORIZONTAL = 0,
  VERTICAL_RL,
  VERTICAL_LR,
};

enum class TextAlignment
{
  TOP_LEFT = 0,
  TOP_CENTER,
  TOP_RIGHT,
  SUB_LEFT,
  SUB_CENTER,
  SUB_RIGHT
};

struct webvttAutoValue
{
  double value = 0;
  bool isAuto = true;
};

struct webvttCueSettings
{
  std::string id;
  std::string regionId;
  WebvttVAlign verticalAlign;
  bool snapToLines;
  webvttAutoValue line;
  webvttAutoValue position;
  WebvttAlign positionAlign;
  double size;
  WebvttAlign align;

  bool operator==(webvttCueSettings const& other) const
  {
    return this->verticalAlign == other.verticalAlign && this->snapToLines == other.snapToLines &&
           this->line.isAuto == other.line.isAuto && this->line.value == other.line.value &&
           this->position.isAuto == other.position.isAuto &&
           this->position.value == other.position.value &&
           this->positionAlign == other.positionAlign && this->size == other.size &&
           this->align == other.align;
  }
};

struct subtitleData
{
  std::string text;
  std::string textRaw; // Text without tags
  webvttCueSettings cueSettings;
  double startTime;
  double stopTime;
  bool useMargins;
  int marginLeft;
  int marginRight;
  int marginVertical;
  TextAlignment textAlign;
};

enum class WebvttSelector
{
  ANY = 0,
  ID,
  TYPE,
  CLASS,
  ATTRIBUTE,
  UNSUPPORTED
};

struct webvttCssStyle
{
  webvttCssStyle() {}
  webvttCssStyle(WebvttSelector selectorType,
                 const std::string& selectorName,
                 const std::string& colorHexRGB)
    : m_selectorType{selectorType},
      m_selectorName{selectorName},
      // Color hex values need to be in BGR format
      m_color(colorHexRGB.substr(4, 2) + colorHexRGB.substr(2, 2) + colorHexRGB.substr(0, 2))
  {
  }

  WebvttSelector m_selectorType = WebvttSelector::ANY;
  std::string m_selectorName;
  std::string m_color;
  bool m_isFontBold = false;
  bool m_isFontItalic = false;
  bool m_isFontUnderline = false;
};

struct tagToken
{
  std::string m_token; // Entire tag
  std::string m_tag;
  std::string m_timestampTag;
  std::string m_annotation;
  std::vector<std::string> m_classes;
  bool m_isClosing;
};

class CWebVTTHandler
{
public:
  CWebVTTHandler(){};
  ~CWebVTTHandler(){};

  /*!
  * \brief Prepare the handler to the decoding
  */
  bool Initialize();

  /*
   * \brief Reset handler data
   */
  void Reset();

  /*!
  * \brief Verify the validity of the WebVTT signature
  */
  bool CheckSignature(const std::string& data);

  /*!
  * \brief Decode a line of the WebVTT text data
  * \param line The line to decode
  * \param subList The list to be filled with decoded subtitles
  */
  void DecodeLine(std::string line, std::vector<subtitleData>* subList);

  /*
   * \brief Return true if the margins are handled by the parser.
   */
  bool IsForcedMargins() const { return !m_overridePositions; }

  /*
   * \brief Set the period start pts to sync subtitles
   */
  void SetPeriodStart(double pts) { m_offset = pts; }

protected:
  void CalculateTextPosition(std::string& subtitleText);
  void ConvertSubtitle(std::string& text);
  void GetCueSettings(std::string& cueSettings);
  subtitleData m_subtitleData;

private:
  bool IsCueLine(std::string& line);
  void GetCueData(std::string& cueText);
  std::string GetCueSettingValue(const std::string& propName,
                                 std::string& text,
                                 const std::string& defaultValue);
  std::string GetCueCssValue(const std::string& cssPropName, std::string& line);
  void AddDefaultCssClasses();
  void InsertCssStyleStartTag(const tagToken& tag,
                              std::string& text,
                              int& pos,
                              int flagTags[],
                              std::deque<std::pair<std::string, webvttCssStyle*>>& cssTagsOpened);
  void InsertCssStyleCloseTag(const tagToken& tag,
                              std::string& text,
                              int& pos,
                              int flagTags[],
                              std::deque<std::pair<std::string, webvttCssStyle*>>& cssTagsOpened,
                              webvttCssStyle& baseStyle);
  bool GetBaseStyle(webvttCssStyle& style);
  void ConvertAddSubtitle(std::vector<subtitleData>* subList);
  void LoadColors();
  double GetTimeFromRegexTS(CRegExp& regex, int indexStart = 1);

  // Last subtitle data added, must persist and be updated between all demuxer packages
  std::unique_ptr<subtitleData> m_lastSubtitleData;

  std::string m_previousLines[3];
  bool m_overrideStyle{false};
  bool m_overridePositions{false};
  WebvttSection m_currentSection{WebvttSection::UNDEFINED};
  CRegExp m_cueTimeRegex;
  CRegExp m_timeRegex;
  std::map<std::string, CRegExp> m_cuePropsMapRegex;
  CGUIColorManager m_colorManager;
  CRegExp m_tagsRegex;
  CRegExp m_cueCssTagRegex;
  std::map<std::string, CRegExp> m_cueCssStyleMapRegex;
  std::vector<std::string> m_feedCssSelectorNames;
  webvttCssStyle m_feedCssStyle;
  std::map<WebvttSelector, std::map<std::string, webvttCssStyle>> m_cssSelectors;

  bool m_CSSColorsLoaded{false};
  std::vector<std::pair<std::string, UTILS::COLOR::ColorInfo>> m_CSSColors;
  double m_offset{0.0};
};
