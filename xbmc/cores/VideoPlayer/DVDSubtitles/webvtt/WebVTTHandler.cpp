/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebVTTHandler.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDSubtitles/SubtitlesStyle.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/CSSUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/HTMLUtil.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>

// This code follow W3C standard https://www.w3.org/TR/webvtt1/
// Due to some Libass rendering limits some feature are not fully implemented
// all these have been documented in the code (like CSS parsing)
// Other special cases are not supported:
// - Cue region (text will be displayed at the bottom)
// - Cue "line" setting as number value
// - Vertical text (used for some specific asian languages only)

using namespace KODI::SUBTITLES;

namespace
{
// WebVTT signature
constexpr const char* signatureCharsBOM = "\xEF\xBB\xBF\x57\x45\x42\x56\x54\x54";
constexpr const char* signatureChars = "\x57\x45\x42\x56\x54\x54";
constexpr char signatureLastChars[] = {'\x0A', '\x0D', '\x20', '\x09'};

constexpr char cueTimePattern[] =
    "^(\\d{2}:)?(\\d{2}):(\\d{2}\\.\\d{3})[ \\t]*-->[ \\t]*(\\d{2}:)?(\\d{2}):(\\d{2}\\.\\d{3})";

// Regex patterns for cue properties
const std::map<std::string, std::string> cuePropsPatterns = {
    {"position", "position\\:(\\d+|\\d+\\.\\d+|auto)%"},
    {"positionAlign", "position\\:\\d+\\.\\d+%,([a-z]+)"},
    {"size", "size\\:((\\d+\\.)?\\d+%?)"},
    {"line", "line\\:(\\d+%|\\d+\\.\\d+%|-?\\d+|auto)(,|\\s|$)"},
    {"align", "align\\:([a-z]+)"},
    {"vertical", "vertical\\:(rl|lr)"},
    {"snapToLines", "snapToLines\\:(true|false)"}};

constexpr char cueCssTagPattern[] = "::cue\\(([^\\(]+)\\)|::cue {";

const std::map<std::string, std::string> cueCssPatterns = {
    {"colorName", "color:\\s*([a-zA-Z]+)(;|\\s)"},
    {"colorRGB", "color:\\s?rgba?\\((\\d{1,3},\\d{1,3},\\d{1,3})(,\\d{1,3})?\\)"},
    {"fontStyle", "font-style:\\s*(italic)(;|\\s)"},
    {"fontWeight", "font-weight:\\s*(bold)(;|\\s)"},
    {"textDecoration", "text-decoration:\\s*(underline)(;|\\s)"}};

const std::map<std::string, std::string> cueCssDefaultColorsClasses = {
    {"c.white", "FFFFFF"},  {"c.lime", "00FF00"},    {"c.cyan", "00FFFF"}, {"c.red", "FF0000"},
    {"c.yellow", "FFFF00"}, {"c.magenta", "FF00FF"}, {"c.blue", "0000FF"}, {"c.black", "000000"}};

enum FlagTags
{
  FLAG_TAG_BOLD = 0,
  FLAG_TAG_ITALIC,
  FLAG_TAG_UNDERLINE,
  FLAG_TAG_CLASS,
  FLAG_TAG_COLOR,
  FLAG_TAG_COUNT
};

bool ValidateSignature(const std::string& data, const char* signature)
{
  const size_t signatureLen = std::strlen(signature);
  if (data.size() > signatureLen)
  {
    if (data.compare(0, signatureLen, signature) == 0)
    {
      // Check if last char is valid
      if (std::strchr(signatureLastChars, data[signatureLen]) != nullptr)
        return true;
    }
  }
  return false;
}

void InsertTextPos(std::string& text, const std::string& insert, int& pos)
{
  text.insert(pos, insert);
  pos += static_cast<int>(insert.length());
}

std::string ConvertStyleToOpenTags(int flagTags[], webvttCssStyle& style)
{
  std::string tags;
  if (style.isFontBold)
  {
    if (flagTags[FLAG_TAG_BOLD] == 0)
      tags += "{\\b1}";
    flagTags[FLAG_TAG_BOLD] += 1;
  }
  if (style.isFontItalic)
  {
    if (flagTags[FLAG_TAG_ITALIC] == 0)
      tags += "{\\i1}";
    flagTags[FLAG_TAG_ITALIC] += 1;
  }
  if (style.isFontUnderline)
  {
    if (flagTags[FLAG_TAG_UNDERLINE] == 0)
      tags += "{\\u1}";
    flagTags[FLAG_TAG_UNDERLINE] += 1;
  }
  if (!style.color.empty())
  {
    if (flagTags[FLAG_TAG_COLOR] > 0)
      tags += "{\\c}";
    flagTags[FLAG_TAG_COLOR] += 1;
    tags += "{\\c&H" + style.color + "&}";
  }
  return tags;
}

std::string ConvertStyleToCloseTags(int flagTags[],
                                    webvttCssStyle& style,
                                    webvttCssStyle& baseStyle)
{
  std::string tags;
  if (style.isFontBold)
  {
    flagTags[FLAG_TAG_BOLD] = flagTags[FLAG_TAG_BOLD] > 0 ? (flagTags[FLAG_TAG_BOLD] - 1) : 0;
    if (flagTags[FLAG_TAG_BOLD] == 0)
      tags += "{\\b0}";
  }
  if (style.isFontItalic)
  {
    flagTags[FLAG_TAG_ITALIC] = flagTags[FLAG_TAG_ITALIC] > 0 ? (flagTags[FLAG_TAG_ITALIC] - 1) : 0;
    if (flagTags[FLAG_TAG_ITALIC] == 0)
      tags += "{\\i0}";
  }
  if (style.isFontUnderline)
  {
    flagTags[FLAG_TAG_UNDERLINE] =
        flagTags[FLAG_TAG_UNDERLINE] > 0 ? (flagTags[FLAG_TAG_UNDERLINE] - 1) : 0;
    if (flagTags[FLAG_TAG_UNDERLINE] == 0)
      tags += "{\\u0}";
  }
  if (!style.color.empty())
  {
    flagTags[FLAG_TAG_COLOR] = flagTags[FLAG_TAG_COLOR] > 0 ? (flagTags[FLAG_TAG_COLOR] - 1) : 0;
    tags += "{\\c}";
    if (flagTags[FLAG_TAG_COLOR] > 0 && !baseStyle.color.empty())
      tags += "{\\c&H" + baseStyle.color + "&}";
  }
  return tags;
}

void TranslateEscapeChars(std::string& text)
{
  if (text.find('&') != std::string::npos)
  {
    StringUtils::Replace(text, "&lrm;", u8"\u200e");
    StringUtils::Replace(text, "&rlm;", u8"\u200f");
    StringUtils::Replace(text, "&#x2068;", u8"\u2068");
    StringUtils::Replace(text, "&#x2069;", u8"\u2069");
    StringUtils::Replace(text, "&amp;", "&");
    StringUtils::Replace(text, "&lt;", "<");
    StringUtils::Replace(text, "&gt;", ">");
    StringUtils::Replace(text, "&nbsp;", " ");
  }
}

constexpr int MICROS_PER_SECOND = 1000000;
constexpr int MPEG_TIMESCALE = 90000;

} // unnamed namespace

bool CWebVTTHandler::Initialize()
{
  m_currentSection = WebvttSection::UNDEFINED;

  // Load default CSS Style values
  AddDefaultCssClasses();

  // Compile regex patterns
  if (!m_tagsRegex.RegComp("<\\/?([^>]*)>"))
    return false;
  if (!m_cueTimeRegex.RegComp(cueTimePattern))
    return false;
  if (!m_cueCssTagRegex.RegComp(cueCssTagPattern))
    return false;
  for (auto const& item : cuePropsPatterns)
  {
    CRegExp reg = CRegExp();
    if (!reg.RegComp(item.second))
      return false;
    m_cuePropsMapRegex.insert({item.first, reg});
  }
  for (auto const& item : cueCssPatterns)
  {
    CRegExp reg = CRegExp();
    if (!reg.RegComp(item.second))
      return false;
    m_cueCssStyleMapRegex.insert({item.first, reg});
  }

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  int overrideStyles = settings->GetInt(CSettings::SETTING_SUBTITLES_OVERRIDESTYLES);
  m_overridePositions = (overrideStyles == (int)KODI::SUBTITLES::OverrideStyles::STYLES_POSITIONS ||
                         overrideStyles == (int)KODI::SUBTITLES::OverrideStyles::POSITIONS);
  m_overrideStyle = (overrideStyles == (int)KODI::SUBTITLES::OverrideStyles::STYLES_POSITIONS ||
                     overrideStyles == (int)KODI::SUBTITLES::OverrideStyles::STYLES);

  return true;
}

bool CWebVTTHandler::CheckSignature(const std::string& data)
{
  // Check the sequence of chars to identify WebVTT signature
  if (ValidateSignature(data, signatureCharsBOM) || ValidateSignature(data, signatureChars))
    return true;

  CLog::Log(LOGERROR, "{} - WebVTT signature not valid", __FUNCTION__);
  return false;
}

void CWebVTTHandler::DecodeLine(std::string line, std::vector<subtitleData>* subList)
{
  // Keep lines values history, needed to identify the cue ID
  m_previousLines[0] = m_previousLines[1];
  m_previousLines[1] = m_previousLines[2];
  m_previousLines[2] = line;

  if (m_currentSection == WebvttSection::UNDEFINED)
  {
    if (line == "STYLE" || line == "Style:") // "Style:" Youtube spec
    {
      m_currentSection = WebvttSection::STYLE;
      return;
    }
    else if (line == "REGION")
    {
      m_currentSection = WebvttSection::REGION;
      return;
    }
    else if (line == "NOTE")
    {
      m_currentSection = WebvttSection::NOTE;
    }
    else if (StringUtils::StartsWith(line, "X-TIMESTAMP-MAP")) // HLS streaming spec
    {
      // Get the HLS timestamp values to sync the subtitles with video
      CRegExp regLocal;
      CRegExp regMpegTs;

      if (regLocal.RegComp("LOCAL:((?:(\\d{1,}):)?(\\d{2}):(\\d{2}\\.\\d{3}))") &&
          regMpegTs.RegComp("MPEGTS:(\\d+)"))
      {
        if ((regLocal.RegFind(line) >= 0 && regLocal.GetSubCount() == 4) &&
            regMpegTs.RegFind(line) >= 0)
        {
          int locHrs = 0;
          int locMins;
          double locSecs;
          if (!regLocal.GetMatch(1).empty())
            locHrs = std::stoi(regLocal.GetMatch(1).c_str());
          locMins = std::stoi(regLocal.GetMatch(2).c_str());
          locSecs = std::atof(regLocal.GetMatch(3).c_str());
          m_hlsTimestampLocalUs =
              (static_cast<double>(locHrs * 3600 + locMins * 60) + locSecs) * DVD_TIME_BASE;
          // Converts a 90 kHz clock timestamp to a timestamp in microseconds
          m_hlsTimestampMpegTsUs =
              std::stod(regMpegTs.GetMatch(1)) * MICROS_PER_SECOND / MPEG_TIMESCALE;
        }
        else
        {
          m_hlsTimestampLocalUs = 0;
          m_hlsTimestampMpegTsUs = 0;
          CLog::Log(LOGERROR,
                    "{} - Failed to get X-TIMESTAMP-MAP values, subtitles could be out of sync",
                    __FUNCTION__);
        }
      }
      else
      {
        CLog::Log(LOGERROR,
                  "{} - Failed to compile X-TIMESTAMP-MAP regexes, subtitles could be out of sync",
                  __FUNCTION__);
      }
    }
    else if (IsCueLine(line))
    {
      // From here we start the cue conversions,
      // other sections should not be allowed with exception of "NOTE"
      m_currentSection = WebvttSection::CUE;
    }
  }

  if (m_currentSection == WebvttSection::CUE || m_currentSection == WebvttSection::CUE_TEXT)
  {
    if (IsCueLine(line))
    {
      if (m_currentSection == WebvttSection::CUE_TEXT && !m_subtitleData.text.empty())
      {
        CLog::Log(LOGWARNING,
                  "{} - Malformed cue, is missing the empty line for the end of cue section",
                  __FUNCTION__);

        // Recover the current cue, add the subtitle to the list
        ConvertSubtitle(m_subtitleData.text);
        subList->emplace_back(m_subtitleData);

        // Change to a new cue section
        m_currentSection = WebvttSection::CUE;
      }
      else if (m_currentSection == WebvttSection::CUE_TEXT)
      {
        CLog::Log(LOGWARNING, "{} - Malformed cue, the cue is within the text area", __FUNCTION__);
        return; // Continue to try again for possible subtitle text after this line
      }

      // Create new subtitle data
      if (m_currentSection == WebvttSection::CUE)
        m_subtitleData = subtitleData();

      // Get and store the cue data
      GetCueData(line);

      // The cue ID is optional, can be a number or a text,
      // used to identify chapters or to apply a specific CSS Style
      if (m_previousLines[0].empty() && !m_previousLines[1].empty())
        m_subtitleData.cueSettings.id = m_previousLines[1];

      // From the next line we should have the text area
      // NOTE: The cue properties will be computed at the first subtitle text line
      m_currentSection = WebvttSection::CUE_TEXT;
      return; // Start read the cue text area from the next line
    }

    if (m_currentSection == WebvttSection::CUE_TEXT)
    {
      if (line.empty()) // An empty line means the end of the current cue
      {
        if (m_subtitleData.text.empty())
        {
          CLog::Log(LOGWARNING, "{} - Malformed cue, is missing the subtitle text", __FUNCTION__);
          m_currentSection = WebvttSection::UNDEFINED;
          return; // This cue will be skipped
        }

        // Add the current subtitle to the list
        // Convert tags and apply the CSS Styles converted
        ConvertSubtitle(m_subtitleData.text);
        subList->emplace_back(m_subtitleData);

        m_currentSection = WebvttSection::CUE;
      }
      else
      {
        if (line == "{")
        {
          // Starting point for JSON metadata
          m_currentSection = WebvttSection::CUE_METADATA;
          return;
        }
        // Collect and convert all subtitle text lines
        if (!m_subtitleData.text.empty())
          m_subtitleData.text += "\n";

        TranslateEscapeChars(line);

        // We need to calculate the text position right here,
        // when we have the first subtitle text line
        if (m_subtitleData.text.empty())
          CalculateTextPosition(line);

        m_subtitleData.text += line;
      }
    }
  }
  else if (m_currentSection == WebvttSection::CUE_METADATA)
  {
    // Cue metadata is not supported (is for scripted apps)
    if (line.empty()) // End of section
      m_currentSection = WebvttSection::UNDEFINED;
  }
  else if (m_currentSection == WebvttSection::STYLE)
  {
    // CSS Styles are not supported in full
    // here there is the support for font styles and colors.
    // Currently supported cue selectors:
    // Cue                            ::cue
    // Type selector                  ::cue(b)
    // Type selector (multiple lines) ::cue(b), ::cue(i), ::cue(c), ...
    // Class selector                 ::cue(.hello)
    // Class selector assigned        ::cue(c.hello)

    if (m_cueCurrentCssStyleSelectors.empty())
      m_feedCssStyle = webvttCssStyle();

    // Collect all CSS cue selectors (can be assigned more than one on multiple lines)
    if (m_cueCssTagRegex.RegFind(line) >= 0)
    {
      std::string selectorName = m_cueCssTagRegex.GetMatch(1);
      UTILS::CSS::Escape(selectorName);
      m_cueCurrentCssStyleSelectors.emplace_back(selectorName);
      // Recover possible css data in-line with the cue
      line =
          line.substr(m_cueCssTagRegex.GetFindLen(), line.length() - m_cueCssTagRegex.GetFindLen());
      if (line.empty())
        line = " "; // To prevent early exit
    }

    // Start parse the CSS Style properties
    if (!m_cueCurrentCssStyleSelectors.empty())
    {
      m_feedCssStyle.selectorName = m_cueCurrentCssStyleSelectors[0];

      // Detect the selector type
      if (m_feedCssStyle.selectorName.empty())
        m_feedCssStyle.selectorType = WebvttSelector::ANY;
      else if (m_feedCssStyle.selectorName.compare(0, 1, "#") == 0)
        m_feedCssStyle.selectorType = WebvttSelector::ID;
      else if (m_feedCssStyle.selectorName.find('.') != std::string::npos)
        m_feedCssStyle.selectorType = WebvttSelector::CLASS;
      else if (m_feedCssStyle.selectorName.find('[') !=
               std::string::npos) // Attribute selector not implemented
        m_feedCssStyle.selectorType = WebvttSelector::UNSUPPORTED;
      else if (m_feedCssStyle.selectorName.compare(0, 1, ":") ==
               0) // Pseudo-classes not implemented
        m_feedCssStyle.selectorType = WebvttSelector::UNSUPPORTED;
      else
        m_feedCssStyle.selectorType = WebvttSelector::TYPE;

      if (m_feedCssStyle.selectorType == WebvttSelector::UNSUPPORTED)
      {
        m_cueCurrentCssStyleSelectors.clear();
        m_currentSection = WebvttSection::UNDEFINED;
        return;
      }

      // Get and store the CSS Style properties
      // Here we collect all text styles data converted as tags,
      // for other implementations that not concern text styles
      // add new variables to webvttCssStyle struct

      // Font color
      std::string colorName = GetCueCssValue("colorName", line);
      if (!colorName.empty()) // From CSS Color name
      {
        if (!m_CSSColorsLoaded) // Load colors in lazy way
          LoadColors();

        auto colorInfo =
            std::find_if(m_CSSColors.begin(), m_CSSColors.end(),
                         [&](const std::pair<std::string, UTILS::COLOR::ColorInfo>& item) {
                           return StringUtils::CompareNoCase(item.first, colorName) == 0;
                         });
        if (colorInfo != m_CSSColors.end())
        {
          uint32_t color = UTILS::COLOR::ConvertToBGR(colorInfo->second.colorARGB);
          m_feedCssStyle.color = StringUtils::Format("{:6x}", color);
        }
      }
      std::string colorRGB = GetCueCssValue("colorRGB", line);
      if (!colorRGB.empty()) // From CSS Color numeric R,G,B values
      {
        auto intValues = StringUtils::Split(colorRGB, ",");
        uint32_t color = UTILS::COLOR::ConvertIntToRGB(
            std::stoi(intValues[2]), std::stoi(intValues[1]), std::stoi(intValues[0]));
        m_feedCssStyle.color = StringUtils::Format("{:6x}", color);
      }
      // Font bold
      if (!GetCueCssValue("fontWeight", line).empty())
        m_feedCssStyle.isFontBold = true;
      // Font italic
      if (!GetCueCssValue("fontStyle", line).empty())
        m_feedCssStyle.isFontItalic = true;
      // Font underline
      if (!GetCueCssValue("textDecoration", line).empty())
        m_feedCssStyle.isFontUnderline = true;
    }

    if (line == "}" || line.empty()) // End of current style
    {
      if (!m_cueCurrentCssStyleSelectors.empty())
      {
        // Store the style
        m_cueCssStyles.emplace_back(m_feedCssStyle);

        if (m_feedCssStyle.selectorType == WebvttSelector::TYPE)
        {
          // If there are multiple type selectors, copy the style for each one
          for (size_t i = 1; i < m_cueCurrentCssStyleSelectors.size(); i++)
          {
            webvttCssStyle cssStyleCopy = m_feedCssStyle;
            cssStyleCopy.selectorName = m_cueCurrentCssStyleSelectors[i];
            m_cueCssStyles.emplace_back(cssStyleCopy);
          }
        }
        m_cueCurrentCssStyleSelectors.clear();
      }
    }
    if (line.empty()) // End of section
    {
      m_cueCurrentCssStyleSelectors.clear();
      m_currentSection = WebvttSection::UNDEFINED;
    }
  }
  else if (m_currentSection == WebvttSection::REGION)
  {
    // Regions are not supported
    if (line.empty()) // End of section
      m_currentSection = WebvttSection::UNDEFINED;
  }
  else if (m_currentSection == WebvttSection::NOTE)
  {
    if (line.empty()) // End of section
      m_currentSection = WebvttSection::UNDEFINED;
  }
}

bool CWebVTTHandler::IsCueLine(std::string& line)
{
  return m_cueTimeRegex.RegFind(line) >= 0;
}

void CWebVTTHandler::GetCueData(std::string& cueText)
{
  std::string cueSettings;
  int sHours = 0;
  int sMinutes;
  double sSeconds;
  int eHours = 0;
  int eMinutes;
  double eSeconds;
  // Valid time formats: (included with or without spaces near -->)
  // 00:00.000 --> 00:00.000
  // 00:00:00.000 --> 00:00:00.000
  if (m_cueTimeRegex.GetSubCount() == 6)
  {
    if (!m_cueTimeRegex.GetMatch(1).empty())
      sHours = std::stoi(m_cueTimeRegex.GetMatch(1).c_str());
    sMinutes = std::stoi(m_cueTimeRegex.GetMatch(2).c_str());
    sSeconds = std::atof(m_cueTimeRegex.GetMatch(3).c_str());
    if (!m_cueTimeRegex.GetMatch(4).empty())
      eHours = std::stoi(m_cueTimeRegex.GetMatch(4).c_str());
    eMinutes = std::stoi(m_cueTimeRegex.GetMatch(5).c_str());
    eSeconds = std::atof(m_cueTimeRegex.GetMatch(6).c_str());

    m_subtitleData.startTime =
        (static_cast<double>(sHours * 3600 + sMinutes * 60) + sSeconds) * DVD_TIME_BASE +
        m_hlsTimestampMpegTsUs - m_hlsTimestampLocalUs;
    m_subtitleData.stopTime =
        (static_cast<double>(eHours * 3600 + eMinutes * 60) + eSeconds) * DVD_TIME_BASE +
        m_hlsTimestampMpegTsUs - m_hlsTimestampLocalUs;
    cueSettings =
        cueText.substr(m_cueTimeRegex.GetFindLen(), cueText.length() - m_cueTimeRegex.GetFindLen());
    StringUtils::Trim(cueSettings);
  }
  else // This should never happen
  {
    CLog::Log(LOGERROR, "{} - Cue timing not found", __FUNCTION__);
  }

  // Parse the cue settings
  GetCueSettings(cueSettings);
}

void CWebVTTHandler::GetCueSettings(std::string& cueSettings)
{
  webvttCueSettings settings;
  // settings.regionId = ""; // "region" is not supported

  std::string cueVertical =
      GetCueSettingValue("vertical", cueSettings, ""); // Ref. Writing direction
  if (cueVertical == "lr")
    settings.verticalAlign = WebvttVAlign::VERTICAL_LR;
  else if (cueVertical == "rl")
    settings.verticalAlign = WebvttVAlign::VERTICAL_RL;
  else
    settings.verticalAlign = WebvttVAlign::HORIZONTAL;

  std::string cuePos = GetCueSettingValue("position", cueSettings, "auto");
  if (cuePos == "auto")
    settings.position.isAuto = true;
  else
  {
    settings.position.isAuto = false;
    settings.position.value = std::stod(cuePos.c_str());
  }

  std::string cuePosAlign = GetCueSettingValue("positionAlign", cueSettings, "auto");
  if (cuePosAlign == "line-left")
    settings.positionAlign = WebvttAlign::LEFT;
  else if (cuePosAlign == "line-right")
    settings.positionAlign = WebvttAlign::RIGHT;
  else if (cuePosAlign == "center" || cuePosAlign == "middle") // "middle" undocumented
    settings.positionAlign = WebvttAlign::CENTER;
  else if (cuePosAlign == "start") // Undocumented
    settings.positionAlign = WebvttAlign::START;
  else if (cuePosAlign == "end") // Undocumented
    settings.positionAlign = WebvttAlign::END;
  else
    settings.positionAlign = WebvttAlign::AUTO;

  std::string cueSize = GetCueSettingValue("size", cueSettings, "100.00");
  settings.size = std::stod(cueSize.c_str());

  std::string cueSnapToLines = GetCueSettingValue("snapToLines", cueSettings, "true");
  settings.snapToLines = cueSnapToLines == "true";

  std::string cueLine = GetCueSettingValue("line", cueSettings, "auto");
  settings.line.isAuto = false;
  auto cueLinePercPos = cueLine.find('%');
  if (cueLine == "auto")
  {
    settings.line.isAuto = true;
    cueLine = "0";
  }
  else if (cueLinePercPos != std::string::npos || !settings.snapToLines)
  {
    settings.snapToLines = false;
    if (cueLinePercPos != std::string::npos)
      cueLine.pop_back(); // Remove % at the end
  }
  settings.line.value = std::stod(cueLine.c_str());

  // The optional "alignment" property of "line" setting is not supported.

  std::string cueAlign = GetCueSettingValue("align", cueSettings, "");
  if (cueAlign == "left")
    settings.align = WebvttAlign::LEFT;
  else if (cueAlign == "right")
    settings.align = WebvttAlign::RIGHT;
  else if (cueAlign == "center" || cueAlign == "middle") // Middle undocumented
    settings.align = WebvttAlign::CENTER;
  else if (cueAlign == "start")
    settings.align = WebvttAlign::START;
  else if (cueAlign == "end")
    settings.align = WebvttAlign::END;
  else
    settings.align = WebvttAlign::CENTER;

  m_subtitleData.cueSettings = settings;
}

void CWebVTTHandler::CalculateTextPosition(std::string& subtitleText)
{
  // Here we cannot handle a kind of cue box, to simulate it we use the margins
  // to position the text on the screen, the subtitle text (horizontal)
  // alignment will be also computed.

  // Sidenote: Limits to bidi direction checks, the webvtt doc specifies that
  // each line of text should be aligned according to the direction of text,
  // but here we limit this by checking the text direction of the first line
  // only, and will be used for all lines of the Cue text.

  int marginLeft = 0;
  int marginRight = 0;
  int marginVertical = 0;
  TextAlignment textAlign;

  webvttCueSettings* settings = &m_subtitleData.cueSettings;

  // Compute cue box "align" value
  if (settings->align == WebvttAlign::START)
  {
    // Clean text from tags or bidi check doesn't work
    std::string textNoTags = subtitleText;
    HTML::CHTMLUtil::RemoveTags(textNoTags);
    if (CCharsetConverter::utf8IsRTLBidiDirection(textNoTags))
      settings->align = WebvttAlign::RIGHT;
    else
      settings->align = WebvttAlign::LEFT;
  }
  else if (settings->align == WebvttAlign::END)
  {
    // Clean text from tags or bidi check doesn't work
    std::string textNoTags = subtitleText;
    HTML::CHTMLUtil::RemoveTags(textNoTags);
    if (CCharsetConverter::utf8IsRTLBidiDirection(textNoTags))
      settings->align = WebvttAlign::LEFT;
    else
      settings->align = WebvttAlign::RIGHT;
  }

  // Compute cue box "position" value
  if (settings->position.isAuto)
  {
    // Position of cue box depends from text alignment
    if (settings->align == WebvttAlign::LEFT)
      settings->position.value = 0;
    else if (settings->align == WebvttAlign::RIGHT)
      settings->position.value = 100;
    else
      settings->position.value = 50;
  }
  int cuePosPixel = static_cast<int>((VIEWPORT_WIDTH / 100) * settings->position.value);

  // Compute cue box "position alignment" value
  if (settings->positionAlign == WebvttAlign::AUTO)
  {
    if (settings->align == WebvttAlign::LEFT)
      settings->positionAlign = WebvttAlign::LEFT; // line-left
    else if (settings->align == WebvttAlign::RIGHT)
      settings->positionAlign = WebvttAlign::RIGHT; // line-right
    else
      settings->positionAlign = WebvttAlign::CENTER;
  }
  else if (settings->positionAlign == WebvttAlign::START) // Undocumented
  {
    // Is not clear if bidi check here is needed
    // Clean text from tags or bidi check doesn't work
    std::string textNoTags = subtitleText;
    HTML::CHTMLUtil::RemoveTags(textNoTags);
    if (CCharsetConverter::utf8IsRTLBidiDirection(textNoTags))
      settings->positionAlign = WebvttAlign::RIGHT; // line-right
    else
      settings->positionAlign = WebvttAlign::LEFT; // line-left
  }
  else if (settings->positionAlign == WebvttAlign::END) // Undocumented
  {
    // Is not clear if bidi check here is needed
    // Clean text from tags or bidi check doesn't work
    std::string textNoTags = subtitleText;
    HTML::CHTMLUtil::RemoveTags(textNoTags);
    if (CCharsetConverter::utf8IsRTLBidiDirection(textNoTags))
      settings->positionAlign = WebvttAlign::LEFT; // line-left
    else
      settings->positionAlign = WebvttAlign::RIGHT; // line-right
  }

  // Compute cue box "size" value
  int cueSizePixel = static_cast<int>((VIEWPORT_WIDTH / 100) * settings->size);

  // Calculate Left/Right margins,
  // by taking into account cue box "position alignment" and cue box "size"
  if (settings->positionAlign == WebvttAlign::LEFT) // line-left
  {
    marginLeft = cuePosPixel;
    marginRight = static_cast<int>(VIEWPORT_WIDTH - (cuePosPixel + cueSizePixel));
  }
  else if (settings->positionAlign == WebvttAlign::RIGHT) // line-right
  {
    marginLeft = static_cast<int>(cuePosPixel - cueSizePixel);
    marginRight = static_cast<int>(VIEWPORT_WIDTH - cuePosPixel);
  }
  else if (settings->positionAlign == WebvttAlign::CENTER)
  {
    int cueHalfSize = static_cast<int>(static_cast<double>(cueSizePixel) / 2);
    marginLeft = cuePosPixel - cueHalfSize;
    marginRight = static_cast<int>(VIEWPORT_WIDTH - (cuePosPixel + cueHalfSize));
  }

  // Compute cue box "line"
  double cueLinePerc = 100.00;
  if (settings->snapToLines) // Numeric line value
  {
    // "line" as numeric value is not supported.
    // From docs is specified to calculate the line position
    // by the height of the first line of text, but is not clear:
    // 1) how we can calculate the position when the text size is not fixed
    // 2) what is the max value and if can change between webvtt files
    // ref. https://www.w3.org/TR/webvtt1/#webvtt-line-cue-setting
  }
  else // Percentage line value
  {
    if (settings->line.value < 0.00)
      cueLinePerc = 0;
    else if (settings->line.value > 100.00)
      cueLinePerc = 100;
    else
      cueLinePerc = settings->line.value;
  }

  // The vertical margin should always referred from the top to simulate
  // the current cue box position without a cue box.
  // But if the vertical margin is too high and the text size is very large,
  // in some cases the text could go off-screen.
  // To try ensure that the text does not go off-screen,
  // above a certain threshold of vertical margin we align from bottom.
  bool useAlignBottom = false;
  if (cueLinePerc >= 90)
  {
    useAlignBottom = true;
    marginVertical = MARGIN_VERTICAL;
  }
  else
    marginVertical = static_cast<int>((VIEWPORT_HEIGHT / 100) * cueLinePerc);

  if (settings->align == WebvttAlign::LEFT)
    textAlign = useAlignBottom ? TextAlignment::SUB_LEFT : TextAlignment::TOP_LEFT;
  else if (settings->align == WebvttAlign::RIGHT)
    textAlign = useAlignBottom ? TextAlignment::SUB_RIGHT : TextAlignment::TOP_RIGHT;
  else
    textAlign = useAlignBottom ? TextAlignment::SUB_CENTER : TextAlignment::TOP_CENTER;

  m_subtitleData.textAlign = textAlign;
  m_subtitleData.useMargins = !m_overridePositions;
  m_subtitleData.marginLeft = marginLeft;
  m_subtitleData.marginRight = marginRight;
  m_subtitleData.marginVertical = marginVertical;
}

std::string CWebVTTHandler::GetCueSettingValue(const std::string& propName,
                                               std::string& text,
                                               std::string defaultValue)
{
  if (m_cuePropsMapRegex[propName].RegFind(text) >= 0)
    return m_cuePropsMapRegex[propName].GetMatch(1);

  return defaultValue;
}

std::string CWebVTTHandler::GetCueCssValue(const std::string& cssPropName, std::string& line)
{
  if (m_cueCssStyleMapRegex[cssPropName].RegFind(line) >= 0)
    return m_cueCssStyleMapRegex[cssPropName].GetMatch(1);

  return "";
}

void CWebVTTHandler::AddDefaultCssClasses()
{
  // Add the default class colors
  for (auto& cClass : cueCssDefaultColorsClasses)
  {
    webvttCssStyle style;
    style.selectorType = WebvttSelector::CLASS;
    style.selectorName = cClass.first;
    // Color hex values need to be in BGR format
    style.color =
        cClass.second.substr(4, 2) + cClass.second.substr(2, 2) + cClass.second.substr(0, 2);
    m_cueCssStyles.emplace_back(style);
  }
}

bool CWebVTTHandler::GetBaseStyle(webvttCssStyle& style)
{
  // Get the style applied to all cue's (type WebvttSelector::ANY) and without name
  bool isBaseStyleFound = false;
  if (!m_overrideStyle)
  {
    auto itBaseStyle =
        std::find_if(m_cueCssStyles.begin(), m_cueCssStyles.end(), FindCssStyleName(""));
    isBaseStyleFound = itBaseStyle != m_cueCssStyles.end();
    if (isBaseStyleFound)
      style = *itBaseStyle;
  }

  // Try find the CSS Style by cue ID (WebvttSelector::ID)
  // and merge it to the base style
  if (!m_subtitleData.cueSettings.id.empty())
  {
    auto itCssStyle = std::find_if(m_cueCssStyles.begin(), m_cueCssStyles.end(),
                                   FindCssStyleName("#" + m_subtitleData.cueSettings.id));
    if (itCssStyle != m_cueCssStyles.end())
    {
      webvttCssStyle& idStyle = *itCssStyle;
      style.isFontBold = style.isFontBold || idStyle.isFontBold;
      style.isFontItalic = style.isFontItalic || idStyle.isFontItalic;
      style.isFontUnderline = style.isFontUnderline || idStyle.isFontUnderline;
      style.color = idStyle.color.empty() ? style.color : idStyle.color;
      return true;
    }
  }
  return isBaseStyleFound;
}

void CWebVTTHandler::ConvertSubtitle(std::string& text)
{
  int pos = 0;
  int flagTags[FLAG_TAG_COUNT] = {0};

  webvttCssStyle baseStyle;
  bool isBaseStyleSet = GetBaseStyle(baseStyle);

  if (isBaseStyleSet)
    text = ConvertStyleToOpenTags(flagTags, baseStyle) + text;

  // Map to store opened CSS tags [tag/class selector]+[style selector name used]
  std::map<std::string, std::string> cssTagsOpened;
  // Scan all tags
  while ((pos = m_tagsRegex.RegFind(text.c_str(), pos)) >= 0)
  {
    std::string fullTag = m_tagsRegex.GetMatch(0);
    StringUtils::ToLower(fullTag);
    // Get tag name only (e.g. full tag is "</c>", tagName will be "c")
    std::string tagName = m_tagsRegex.GetMatch(1);

    text.erase(pos, fullTag.length());

    if (fullTag.substr(1, 1) == "/")
      InsertCssStyleCloseTag(tagName, text, pos, flagTags, cssTagsOpened, baseStyle);

    if (fullTag == "<b>" || StringUtils::StartsWith(fullTag, "<b."))
    {
      if (flagTags[FLAG_TAG_BOLD] == 0)
        InsertTextPos(text, "{\\b1}", pos);
      flagTags[FLAG_TAG_BOLD] += 1;
    }
    else if (fullTag == "</b>" && flagTags[FLAG_TAG_BOLD] > 0)
    {
      flagTags[FLAG_TAG_BOLD] = flagTags[FLAG_TAG_BOLD] > 0 ? (flagTags[FLAG_TAG_BOLD] - 1) : 0;
      if (flagTags[FLAG_TAG_BOLD] == 0)
        InsertTextPos(text, "{\\b0}", pos);
    }
    else if (fullTag == "<i>" || StringUtils::StartsWith(fullTag, "<i."))
    {
      if (flagTags[FLAG_TAG_ITALIC] == 0)
        InsertTextPos(text, "{\\i1}", pos);
      flagTags[FLAG_TAG_ITALIC] += 1;
    }
    else if (fullTag == "</i>" && flagTags[FLAG_TAG_ITALIC] > 0)
    {
      flagTags[FLAG_TAG_ITALIC] =
          flagTags[FLAG_TAG_ITALIC] > 0 ? (flagTags[FLAG_TAG_ITALIC] - 1) : 0;
      if (flagTags[FLAG_TAG_ITALIC] == 0)
        InsertTextPos(text, "{\\i0}", pos);
    }
    else if (fullTag == "<u>" || StringUtils::StartsWith(fullTag, "<u."))
    {
      if (flagTags[FLAG_TAG_UNDERLINE] == 0)
        InsertTextPos(text, "{\\u1}", pos);
      flagTags[FLAG_TAG_UNDERLINE] += 1;
    }
    else if (fullTag == "</u>" && flagTags[FLAG_TAG_UNDERLINE] > 0)
    {
      flagTags[FLAG_TAG_UNDERLINE] =
          flagTags[FLAG_TAG_UNDERLINE] > 0 ? (flagTags[FLAG_TAG_UNDERLINE] - 1) : 0;
      if (flagTags[FLAG_TAG_UNDERLINE] == 0)
        InsertTextPos(text, "{\\u0}", pos);
    }

    if (fullTag.substr(1, 1) != "/")
      InsertCssStyleStartTag(tagName, text, pos, flagTags, cssTagsOpened);
  }

  if (isBaseStyleSet)
  {
    webvttCssStyle emptyStyle;
    text += ConvertStyleToCloseTags(flagTags, baseStyle, emptyStyle);
  }

  // Check for malformed tags still opened
  if (!cssTagsOpened.empty() || flagTags[FLAG_TAG_BOLD] > 0 || flagTags[FLAG_TAG_ITALIC] > 0 ||
      flagTags[FLAG_TAG_UNDERLINE] > 0)
  {
    text += "{\\r}"; // Cancel all opened tags
  }

  // Add text alignment (based on cue settings)
  if (!m_overridePositions)
  {
    if (m_subtitleData.textAlign == TextAlignment::TOP_LEFT)
      text.insert(0, "{\\an7}");
    else if (m_subtitleData.textAlign == TextAlignment::TOP_RIGHT)
      text.insert(0, "{\\an9}");
    else if (m_subtitleData.textAlign == TextAlignment::TOP_CENTER)
      text.insert(0, "{\\an8}");
    else if (m_subtitleData.textAlign == TextAlignment::SUB_LEFT)
      text.insert(0, "{\\an1}");
    else if (m_subtitleData.textAlign == TextAlignment::SUB_RIGHT)
      text.insert(0, "{\\an3}");
    else if (m_subtitleData.textAlign == TextAlignment::SUB_CENTER)
      text.insert(0, "{\\an2}");
  }
}

void CWebVTTHandler::InsertCssStyleStartTag(std::string& tagName,
                                            std::string& text,
                                            int& pos,
                                            int flagTags[],
                                            std::map<std::string, std::string>& cssTagsOpened)
{
  // Get class selector (e.g. full tag is "<c.loud>", the class selection will be ".loud")
  std::string classSelectorName;
  auto dotPos = tagName.find('.');
  if (dotPos != std::string::npos)
    classSelectorName = tagName.substr(dotPos, tagName.length() - dotPos);

  auto itCssStyle =
      std::find_if(m_cueCssStyles.begin(), m_cueCssStyles.end(), FindCssStyleName(tagName));
  if (itCssStyle == m_cueCssStyles.end() && !classSelectorName.empty())
    itCssStyle = std::find_if(m_cueCssStyles.begin(), m_cueCssStyles.end(),
                              FindCssStyleName(classSelectorName));
  if (itCssStyle != m_cueCssStyles.end())
  {
    // Insert the CSS Style converted as tags
    auto& cssStyle = *itCssStyle;
    std::string tags = ConvertStyleToOpenTags(flagTags, cssStyle);
    text.insert(pos, tags);
    pos += static_cast<int>(tags.length());
    // Keep track of the opened tag to be closed
    // or when we have to insert the closing tags we do not know what style we are closing
    cssTagsOpened.insert({tagName, cssStyle.selectorName});
  }
}

void CWebVTTHandler::InsertCssStyleCloseTag(std::string& tagName,
                                            std::string& text,
                                            int& pos,
                                            int flagTags[],
                                            std::map<std::string, std::string>& cssTagsOpened,
                                            webvttCssStyle& baseStyle)
{
  auto itCssTagToClose = cssTagsOpened.find(tagName);
  if (itCssTagToClose != cssTagsOpened.end())
  {
    // Get the style used to open the tag
    auto itCssStyle = std::find_if(m_cueCssStyles.begin(), m_cueCssStyles.end(),
                                   FindCssStyleName(itCssTagToClose->second));
    if (itCssStyle != m_cueCssStyles.end())
    {
      std::string tags = ConvertStyleToCloseTags(flagTags, *itCssStyle, baseStyle);
      text.insert(pos, tags);
      pos += static_cast<int>(tags.length());
      cssTagsOpened.erase(itCssTagToClose);
    }
  }
}

void CWebVTTHandler::LoadColors()
{
  CGUIColorManager colorManager;
  colorManager.LoadColorsListFromXML(
      CSpecialProtocol::TranslatePathConvertCase("special://xbmc/system/colors.xml"), m_CSSColors,
      false);
  m_CSSColorsLoaded = true;
}
