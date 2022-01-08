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

#include <map>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>


enum class WebvttSection
{
  UNDEFINED = 0,
  STYLE,
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
};

struct subtitleData
{
  std::string text;
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
  UNSUPPORTED
};

struct webvttCssStyle
{
  WebvttSelector selectorType;
  std::string selectorName;
  std::string color;
  bool isFontBold = false;
  bool isFontItalic = false;
  bool isFontUnderline = false;
};

class FindCssStyleName
{
public:
  FindCssStyleName(std::string name) : m_name(std::move(name)) {}
  bool operator()(const webvttCssStyle& other) const { return other.selectorName == m_name; }

private:
  std::string m_name;
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
                                 std::string defaultValue);
  std::string GetCueCssValue(const std::string& cssPropName, std::string& line);
  void AddDefaultCssClasses();
  void InsertCssStyleStartTag(std::string& tagName,
                              std::string& text,
                              int& pos,
                              int flagTags[],
                              std::map<std::string, std::string>& cssTagsOpened);
  void InsertCssStyleCloseTag(std::string& tagName,
                              std::string& text,
                              int& pos,
                              int flagTags[],
                              std::map<std::string, std::string>& cssTagsOpened,
                              webvttCssStyle& baseStyle);
  bool GetBaseStyle(webvttCssStyle& style);
  void LoadColors();

  std::string m_previousLines[3];
  bool m_overrideStyle{false};
  bool m_overridePositions{false};
  WebvttSection m_currentSection{WebvttSection::UNDEFINED};
  double m_hlsTimestampMpegTsUs{0};
  double m_hlsTimestampLocalUs{0};
  CRegExp m_cueTimeRegex;
  std::map<std::string, CRegExp> m_cuePropsMapRegex;
  CGUIColorManager m_colorManager;
  CRegExp m_tagsRegex;
  CRegExp m_cueCssTagRegex;
  std::map<std::string, CRegExp> m_cueCssStyleMapRegex;
  std::vector<std::string> m_cueCurrentCssStyleSelectors;
  webvttCssStyle m_feedCssStyle;
  std::vector<webvttCssStyle> m_cueCssStyles;
  bool m_CSSColorsLoaded{false};
  std::vector<std::pair<std::string, UTILS::COLOR::ColorInfo>> m_CSSColors;
};
