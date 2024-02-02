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
#include "settings/SettingsComponent.h"
#include "settings/SubtitlesSettings.h"
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

using namespace KODI::SUBTITLES::STYLE;

namespace
{
// WebVTT signature
constexpr const char* signatureCharsBOM = "\xEF\xBB\xBF\x57\x45\x42\x56\x54\x54";
constexpr const char* signatureChars = "\x57\x45\x42\x56\x54\x54";
constexpr char signatureLastChars[] = {'\x0A', '\x0D', '\x20', '\x09'};

constexpr char tagPattern[] = "<(\\/)?([^a-zA-Z >]+)?([^\\d:. >]+)?(\\.[^ >]+)?(?> ([^>]+))?>";

constexpr char cueTimePattern[] = "^(?>(\\d{2,}):)?(\\d{2}):(\\d{2}\\.\\d{3})"
                                  "[ \\t]*-->[ \\t]*"
                                  "(?>(\\d{2,}):)?(\\d{2}):(\\d{2}\\.\\d{3})";

constexpr char timePattern[] = "<(?>(\\d{2,}):)?(\\d{2}):(\\d{2}\\.\\d{3})>";

// Regex patterns for cue properties
const std::map<std::string, std::string> cuePropsPatterns = {
    {"position", "position\\:(\\d+|\\d+\\.\\d+|auto)%"},
    {"positionAlign", "position\\:\\d+\\.\\d+%,([a-z]+)"},
    {"size", "size\\:((\\d+\\.)?\\d+%?)"},
    {"line", "line\\:(\\d+%|\\d+\\.\\d+%|-?\\d+|auto)(,|\\s|$)"},
    {"align", "align\\:([a-z]+)"},
    {"vertical", "vertical\\:(rl|lr)"},
    {"snapToLines", "snapToLines\\:(true|false)"}};

constexpr char cueCssTagPattern[] = "::cue\\(([^\\(]+)\\)|(?>(::cue)\\(?\\)?) *{";

const std::map<std::string, std::string> cueCssPatterns = {
    {"colorName", "color:\\s*([a-zA-Z]+)($|;|\\s|})"},
    {"colorRGB", "color:\\s?rgba?\\((\\d{1,3},\\d{1,3},\\d{1,3})(,\\d{1,3})?\\)"},
    {"fontStyle", "font-style:\\s*(italic)($|;|\\s|})"},
    {"fontWeight", "font-weight:\\s*(bold)($|;|\\s|})"},
    {"textDecoration", "text-decoration:\\s*(underline)($|;|\\s|})"}};

const std::map<std::string, webvttCssStyle> cueCssDefaultColorClasses = {
    {".white", {WebvttSelector::CLASS, ".white", "FFFFFF"}},
    {".lime", {WebvttSelector::CLASS, ".lime", "00FF00"}},
    {".cyan", {WebvttSelector::CLASS, ".cyan", "00FFFF"}},
    {".red", {WebvttSelector::CLASS, ".red", "FF0000"}},
    {".yellow", {WebvttSelector::CLASS, ".yellow", "FFFF00"}},
    {".magenta", {WebvttSelector::CLASS, ".magenta", "FF00FF"}},
    {".blue", {WebvttSelector::CLASS, ".blue", "0000FF"}},
    {".black", {WebvttSelector::CLASS, ".black", "000000"}}};

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
      if (std::memchr(signatureLastChars, data[signatureLen], sizeof(signatureLastChars)) !=
          nullptr)
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
  if (style.m_isFontBold)
  {
    if (flagTags[FLAG_TAG_BOLD] == 0)
      tags += "{\\b1}";
    flagTags[FLAG_TAG_BOLD] += 1;
  }
  if (style.m_isFontItalic)
  {
    if (flagTags[FLAG_TAG_ITALIC] == 0)
      tags += "{\\i1}";
    flagTags[FLAG_TAG_ITALIC] += 1;
  }
  if (style.m_isFontUnderline)
  {
    if (flagTags[FLAG_TAG_UNDERLINE] == 0)
      tags += "{\\u1}";
    flagTags[FLAG_TAG_UNDERLINE] += 1;
  }
  if (!style.m_color.empty())
  {
    if (flagTags[FLAG_TAG_COLOR] > 0)
      tags += "{\\c}";
    flagTags[FLAG_TAG_COLOR] += 1;
    tags += "{\\c&H" + style.m_color + "&}";
  }
  return tags;
}

std::string ConvertStyleToCloseTags(int flagTags[],
                                    webvttCssStyle* style,
                                    webvttCssStyle* baseStyle)
{
  std::string tags;
  if (style->m_isFontBold)
  {
    flagTags[FLAG_TAG_BOLD] = flagTags[FLAG_TAG_BOLD] > 0 ? (flagTags[FLAG_TAG_BOLD] - 1) : 0;
    if (flagTags[FLAG_TAG_BOLD] == 0)
      tags += "{\\b0}";
  }
  if (style->m_isFontItalic)
  {
    flagTags[FLAG_TAG_ITALIC] = flagTags[FLAG_TAG_ITALIC] > 0 ? (flagTags[FLAG_TAG_ITALIC] - 1) : 0;
    if (flagTags[FLAG_TAG_ITALIC] == 0)
      tags += "{\\i0}";
  }
  if (style->m_isFontUnderline)
  {
    flagTags[FLAG_TAG_UNDERLINE] =
        flagTags[FLAG_TAG_UNDERLINE] > 0 ? (flagTags[FLAG_TAG_UNDERLINE] - 1) : 0;
    if (flagTags[FLAG_TAG_UNDERLINE] == 0)
      tags += "{\\u0}";
  }
  if (!style->m_color.empty())
  {
    flagTags[FLAG_TAG_COLOR] = flagTags[FLAG_TAG_COLOR] > 0 ? (flagTags[FLAG_TAG_COLOR] - 1) : 0;
    tags += "{\\c}";
    if (flagTags[FLAG_TAG_COLOR] > 0 && !baseStyle->m_color.empty())
      tags += "{\\c&H" + baseStyle->m_color + "&}";
  }
  return tags;
}

void TranslateEscapeChars(std::string& text)
{
  if (text.find('&') != std::string::npos)
  {
    // The specs says to use unicode
    // U+200E for "&lrm;" and U+200F for "&rlm;"
    // but libass rendering assume the text as left-to-right,
    // to display text in the right order we have to use embedded codes
    StringUtils::Replace(text, "&lrm;", u8"\u202a");
    StringUtils::Replace(text, "&rlm;", u8"\u202b");
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
  if (!m_tagsRegex.RegComp(tagPattern))
    return false;
  if (!m_cueTimeRegex.RegComp(cueTimePattern))
    return false;
  if (!m_timeRegex.RegComp(timePattern))
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

  auto overrideStyles{
      CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->GetOverrideStyles()};
  m_overridePositions = (overrideStyles == KODI::SUBTITLES::OverrideStyles::STYLES_POSITIONS ||
                         overrideStyles == KODI::SUBTITLES::OverrideStyles::POSITIONS);
  m_overrideStyle = (overrideStyles == KODI::SUBTITLES::OverrideStyles::STYLES_POSITIONS ||
                     overrideStyles == KODI::SUBTITLES::OverrideStyles::STYLES);

  return true;
}

void CWebVTTHandler::Reset()
{
  m_previousLines[0].clear();
  m_previousLines[1].clear();
  m_previousLines[2].clear();
  m_currentSection = WebvttSection::UNDEFINED;
  m_offset = 0;
}

bool CWebVTTHandler::CheckSignature(const std::string& data)
{
  // Check the sequence of chars to identify WebVTT signature
  if (ValidateSignature(data, signatureCharsBOM) || ValidateSignature(data, signatureChars))
    return true;

  CLog::LogF(LOGERROR, "WebVTT signature not valid");
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
        double tsLocalUs{0.0};
        double tsMpegUs{0.0};

        if ((regLocal.RegFind(line) >= 0 && regLocal.GetSubCount() == 4) &&
            regMpegTs.RegFind(line) >= 0)
        {
          tsLocalUs = GetTimeFromRegexTS(regLocal, 2);
          // Converts a 90 kHz clock timestamp to a timestamp in microseconds
          tsMpegUs = std::stod(regMpegTs.GetMatch(1)) * MICROS_PER_SECOND / MPEG_TIMESCALE;
        }
        else
        {
          CLog::LogF(LOGERROR,
                     "Failed to get X-TIMESTAMP-MAP values, subtitles could be out of sync");
        }

        // offset = periodStart + tsMpegUs - tsLocalUs
        m_offset += tsMpegUs - tsLocalUs;
      }
      else
      {
        CLog::LogF(LOGERROR,
                   "Failed to compile X-TIMESTAMP-MAP regexes, subtitles could be out of sync");
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
        CLog::LogF(LOGWARNING,
                   "Malformed cue, is missing the empty line for the end of cue section");

        // Recover the current cue, add the subtitle to the list
        ConvertAddSubtitle(subList);

        // Change to a new cue section
        m_currentSection = WebvttSection::CUE;
      }
      else if (m_currentSection == WebvttSection::CUE_TEXT)
      {
        CLog::LogF(LOGWARNING, "Malformed cue, the cue is within the text area");
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
          CLog::LogF(LOGWARNING, "Malformed cue, is missing the subtitle text");
          m_currentSection = WebvttSection::UNDEFINED;
          return; // This cue will be skipped
        }

        ConvertAddSubtitle(subList);

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
  else if (m_currentSection == WebvttSection::STYLE ||
           m_currentSection == WebvttSection::STYLE_CONTENT)
  {
    // Non-implemented CSS selector features:
    // - Attribute selector [lang="xx-yy"] for applicable language
    // - Pseudo-classes
    // - Cue region
    // - Cascading classes for color-background (takes only first color)

    if (line.empty()) // End of section
    {
      m_feedCssSelectorNames.clear();
      m_currentSection = WebvttSection::UNDEFINED;
    }

    if (m_currentSection == WebvttSection::STYLE)
    {
      if (m_feedCssSelectorNames.empty())
        m_feedCssStyle = webvttCssStyle();

      // Collect cue selectors (also handle multiple inline selectors)
      for (std::string& cueSelector : StringUtils::Split(line, ','))
      {
        if (m_cueCssTagRegex.RegFind(cueSelector) >= 0)
        {
          std::string selectorName = m_cueCssTagRegex.GetMatch(m_cueCssTagRegex.GetSubCount());
          UTILS::CSS::Escape(selectorName);
          m_feedCssSelectorNames.emplace_back(selectorName);
        }
      }

      if (line.find('{') != std::string::npos && !m_feedCssSelectorNames.empty())
      {
        // Detect the selector type, from the first selector name
        std::string_view selectorName = m_feedCssSelectorNames[0];

        if (selectorName == "::cue")
          m_feedCssStyle.m_selectorType = WebvttSelector::ANY;
        else if (selectorName[0] == '#')
          m_feedCssStyle.m_selectorType = WebvttSelector::ID;
        else if (selectorName.find('.') != std::string::npos)
          m_feedCssStyle.m_selectorType = WebvttSelector::CLASS;
        else if (selectorName.compare(0, 9, "lang[lang") == 0 ||
                 selectorName.compare(0, 7, "v[voice") == 0)
          m_feedCssStyle.m_selectorType = WebvttSelector::ATTRIBUTE;
        else if (selectorName[0] == ':') // Pseudo-classes not implemented
          m_feedCssStyle.m_selectorType = WebvttSelector::UNSUPPORTED;
        else
          m_feedCssStyle.m_selectorType = WebvttSelector::TYPE;

        if (m_feedCssStyle.m_selectorType == WebvttSelector::UNSUPPORTED)
        {
          m_feedCssSelectorNames.clear();
          m_currentSection = WebvttSection::UNDEFINED;
          return;
        }

        // Go through to recover possible data inline with the selector
        m_currentSection = WebvttSection::STYLE_CONTENT;
      }
    }

    if (m_currentSection == WebvttSection::STYLE_CONTENT)
    {
      // Get and store the CSS Style properties
      // Font color
      const std::string colorName = GetCueCssValue("colorName", line);
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
          const uint32_t color = UTILS::COLOR::ConvertToBGR(colorInfo->second.colorARGB);
          m_feedCssStyle.m_color = StringUtils::Format("{:6x}", color);
        }
      }
      std::string colorRGB = GetCueCssValue("colorRGB", line);
      if (!colorRGB.empty()) // From CSS Color numeric R,G,B values
      {
        const auto intValues = StringUtils::Split(colorRGB, ",");
        uint32_t color = UTILS::COLOR::ConvertIntToRGB(
            std::stoi(intValues[2]), std::stoi(intValues[1]), std::stoi(intValues[0]));
        m_feedCssStyle.m_color = StringUtils::Format("{:6x}", color);
      }
      // Font bold
      if (!GetCueCssValue("fontWeight", line).empty())
        m_feedCssStyle.m_isFontBold = true;
      // Font italic
      if (!GetCueCssValue("fontStyle", line).empty())
        m_feedCssStyle.m_isFontItalic = true;
      // Font underline
      if (!GetCueCssValue("textDecoration", line).empty())
        m_feedCssStyle.m_isFontUnderline = true;

      if (line.find('}') != std::string::npos || line.empty()) // End of current style
      {
        // Store the style
        // Overwrite existing selectors to allow authors to change default classes
        auto& selectorTypeMap = m_cssSelectors[m_feedCssStyle.m_selectorType];
        // For multiple selectors, copy the style for each one
        for (std::string_view selectorName : m_feedCssSelectorNames)
        {
          webvttCssStyle cssStyleCopy = m_feedCssStyle;
          // Convert CSS syntax to WebVTT syntax selector
          std::string selectorNameConv{selectorName};
          if (m_feedCssStyle.m_selectorType == WebvttSelector::ATTRIBUTE)
          {
            selectorNameConv = selectorName.substr(0, selectorName.find('['));
            selectorNameConv += " ";
            const size_t attribPosStart = selectorName.find('"');
            const size_t attribPosEnd = selectorName.find_last_of('"');
            if (attribPosEnd > attribPosStart)
            {
              selectorNameConv +=
                  selectorName.substr(attribPosStart + 1, attribPosEnd - 1 - attribPosStart);
            }
          }
          else if (m_feedCssStyle.m_selectorType == WebvttSelector::ID)
            selectorNameConv.erase(0, 1); // Remove # char

          cssStyleCopy.m_selectorName = selectorNameConv;
          selectorTypeMap[selectorNameConv] = cssStyleCopy;
        }
        m_feedCssSelectorNames.clear();

        // Let's go back to "STYLE" to parse multiple "::cue" on the same section
        m_currentSection = WebvttSection::STYLE;
      }
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
  // Valid time formats: (included with or without spaces near -->)
  // 00:00.000 --> 00:00.000
  // 00:00:00.000 --> 00:00:00.000
  if (m_cueTimeRegex.GetSubCount() == 6)
  {
    m_subtitleData.startTime = GetTimeFromRegexTS(m_cueTimeRegex) + m_offset;
    m_subtitleData.stopTime = GetTimeFromRegexTS(m_cueTimeRegex, 4) + m_offset;
    cueSettings =
        cueText.substr(m_cueTimeRegex.GetFindLen(), cueText.length() - m_cueTimeRegex.GetFindLen());
    StringUtils::Trim(cueSettings);
  }
  else // This should never happen
  {
    CLog::LogF(LOGERROR, "Cue timing not found");
  }

  // Parse the cue settings
  GetCueSettings(cueSettings);
}

void CWebVTTHandler::GetCueSettings(std::string& cueSettings)
{
  webvttCueSettings settings;
  // settings.regionId = ""; // "region" is not supported

  const std::string cueVertical =
      GetCueSettingValue("vertical", cueSettings, ""); // Ref. Writing direction
  if (cueVertical == "lr")
    settings.verticalAlign = WebvttVAlign::VERTICAL_LR;
  else if (cueVertical == "rl")
    settings.verticalAlign = WebvttVAlign::VERTICAL_RL;
  else
    settings.verticalAlign = WebvttVAlign::HORIZONTAL;

  const std::string cuePos = GetCueSettingValue("position", cueSettings, "auto");
  if (cuePos == "auto")
    settings.position.isAuto = true;
  else
  {
    settings.position.isAuto = false;
    settings.position.value = std::stod(cuePos.c_str());
    if (settings.position.value > 100) // Not valid
      settings.position.value = 50;
  }

  const std::string cuePosAlign = GetCueSettingValue("positionAlign", cueSettings, "auto");
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

  const std::string cueSize = GetCueSettingValue("size", cueSettings, "100.00");
  settings.size = std::stod(cueSize.c_str());
  if (settings.size > 100.0) // Not valid
    settings.size = 100.0;

  const std::string cueSnapToLines = GetCueSettingValue("snapToLines", cueSettings, "true");
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

  const std::string cueAlign = GetCueSettingValue("align", cueSettings, "");
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
  m_subtitleData.marginLeft = std::max(marginLeft, 0);
  m_subtitleData.marginRight = std::max(marginRight, 0);
  m_subtitleData.marginVertical = marginVertical;
}

std::string CWebVTTHandler::GetCueSettingValue(const std::string& propName,
                                               std::string& text,
                                               const std::string& defaultValue)
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
  m_cssSelectors[WebvttSelector::CLASS] = cueCssDefaultColorClasses;
}

bool CWebVTTHandler::GetBaseStyle(webvttCssStyle& style)
{
  // Get the style applied to all cue's
  bool isBaseStyleFound = false;
  if (!m_overrideStyle)
  {
    auto& selectorAnyMap = m_cssSelectors[WebvttSelector::ANY];
    auto itBaseStyle = selectorAnyMap.find("::cue");
    if (itBaseStyle != selectorAnyMap.end())
    {
      style = itBaseStyle->second;
      isBaseStyleFound = true;
    }
  }

  // Try find the CSS Style by cue ID
  // and merge it to the base style
  if (!m_subtitleData.cueSettings.id.empty())
  {
    auto& selectorIdMap = m_cssSelectors[WebvttSelector::ID];
    auto itCssStyle = selectorIdMap.find(m_subtitleData.cueSettings.id);
    if (itCssStyle != selectorIdMap.end())
    {
      webvttCssStyle& idStyle = itCssStyle->second;
      style.m_isFontBold = style.m_isFontBold || idStyle.m_isFontBold;
      style.m_isFontItalic = style.m_isFontItalic || idStyle.m_isFontItalic;
      style.m_isFontUnderline = style.m_isFontUnderline || idStyle.m_isFontUnderline;
      style.m_color = idStyle.m_color.empty() ? style.m_color : idStyle.m_color;
      return true;
    }
  }
  return isBaseStyleFound;
}

void CWebVTTHandler::ConvertSubtitle(std::string& text)
{
  int pos = 0;
  int flagTags[FLAG_TAG_COUNT] = {0};

  std::string textRaw;
  int lastPos{0};
  webvttCssStyle baseStyle;
  bool isBaseStyleSet = GetBaseStyle(baseStyle);

  if (isBaseStyleSet)
  {
    const std::string baseStyleTag = ConvertStyleToOpenTags(flagTags, baseStyle);
    text.insert(0, baseStyleTag);
    lastPos = baseStyleTag.length();
  }

  // Map to store opened CSS tags [tag name]+[style selector]
  std::deque<std::pair<std::string, webvttCssStyle*>> cssTagsOpened;
  // Scan all tags
  while ((pos = m_tagsRegex.RegFind(text, pos)) >= 0)
  {
    tagToken tag;
    tag.m_token = StringUtils::ToLower(m_tagsRegex.GetMatch(0));
    tag.m_isClosing = m_tagsRegex.GetMatch(1) == "/";
    if (!m_tagsRegex.GetMatch(2).empty())
      tag.m_timestampTag = tag.m_token;
    tag.m_tag = StringUtils::ToLower(m_tagsRegex.GetMatch(3));
    tag.m_classes = StringUtils::Split(m_tagsRegex.GetMatch(4).erase(0, 1), ".");
    tag.m_annotation = m_tagsRegex.GetMatch(5);

    text.erase(pos, tag.m_token.length());
    // Keep a copy of the text without tags
    textRaw += text.substr(lastPos, pos - lastPos);

    if (tag.m_isClosing)
      InsertCssStyleCloseTag(tag, text, pos, flagTags, cssTagsOpened, baseStyle);

    if (tag.m_tag == "b")
    {
      if (!tag.m_isClosing)
      {
        if (flagTags[FLAG_TAG_BOLD] == 0)
          InsertTextPos(text, "{\\b1}", pos);
        flagTags[FLAG_TAG_BOLD] += 1;
      }
      else if (flagTags[FLAG_TAG_BOLD] > 0)
      { // Closing tag (if previously opened)
        flagTags[FLAG_TAG_BOLD] = flagTags[FLAG_TAG_BOLD] > 0 ? (flagTags[FLAG_TAG_BOLD] - 1) : 0;
        if (flagTags[FLAG_TAG_BOLD] == 0)
          InsertTextPos(text, "{\\b0}", pos);
      }
    }
    else if (tag.m_tag == "i")
    {
      if (!tag.m_isClosing)
      {
        if (flagTags[FLAG_TAG_ITALIC] == 0)
          InsertTextPos(text, "{\\i1}", pos);
        flagTags[FLAG_TAG_ITALIC] += 1;
      }
      else if (flagTags[FLAG_TAG_ITALIC] > 0)
      { // Closing tag (if previously opened)
        flagTags[FLAG_TAG_ITALIC] =
            flagTags[FLAG_TAG_ITALIC] > 0 ? (flagTags[FLAG_TAG_ITALIC] - 1) : 0;
        if (flagTags[FLAG_TAG_ITALIC] == 0)
          InsertTextPos(text, "{\\i0}", pos);
      }
    }
    else if (tag.m_tag == "u")
    {
      if (!tag.m_isClosing)
      {
        if (flagTags[FLAG_TAG_UNDERLINE] == 0)
          InsertTextPos(text, "{\\u1}", pos);
        flagTags[FLAG_TAG_UNDERLINE] += 1;
      }
      else if (flagTags[FLAG_TAG_ITALIC] > 0)
      { // Closing tag (if previously opened)
        flagTags[FLAG_TAG_UNDERLINE] =
            flagTags[FLAG_TAG_UNDERLINE] > 0 ? (flagTags[FLAG_TAG_UNDERLINE] - 1) : 0;
        if (flagTags[FLAG_TAG_UNDERLINE] == 0)
          InsertTextPos(text, "{\\u0}", pos);
      }
    }

    if (!tag.m_isClosing)
      InsertCssStyleStartTag(tag, text, pos, flagTags, cssTagsOpened);

    lastPos = pos;
  }
  // Keep a copy of the text without tags
  textRaw += text.substr(lastPos);

  m_subtitleData.textRaw = textRaw;

  if (isBaseStyleSet)
  {
    webvttCssStyle emptyStyle{};
    text += ConvertStyleToCloseTags(flagTags, &baseStyle, &emptyStyle);
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

void CWebVTTHandler::InsertCssStyleStartTag(
    const tagToken& tag,
    std::string& text,
    int& pos,
    int flagTags[],
    std::deque<std::pair<std::string, webvttCssStyle*>>& cssTagsOpened)
{
  if (!tag.m_timestampTag.empty())
  {
    // Timestamp tag will be interpreded as karaoke effect
    if (m_timeRegex.RegFind(tag.m_timestampTag) >= 0)
    {
      const double timeStart = GetTimeFromRegexTS(m_timeRegex) + m_offset;
      // Libass works with duration instead of timestamp
      // so we need to find the next timestamp
      double timeEnd = m_subtitleData.stopTime;
      if (m_timeRegex.RegFind(text) >= 0)
        timeEnd = GetTimeFromRegexTS(m_timeRegex) + m_offset;

      if (timeStart <= timeEnd)
      {
        int duration = static_cast<int>(timeEnd - timeStart) / 10000;
        std::string assTag = "{\\k" + std::to_string(duration) + "}";
        text.insert(pos, assTag);
        pos += static_cast<int>(assTag.length());
      }
      else
        CLog::LogF(LOGERROR, "Unable to get duration from timestamp: {}", tag.m_timestampTag);
    }
    else
      CLog::LogF(LOGERROR, "Error parsing timestamp tag: {}", tag.m_timestampTag);

    return;
  }

  bool hasAttribute = !tag.m_annotation.empty() && (tag.m_tag == "lang" || tag.m_tag == "v");

  webvttCssStyle* cssStyle{nullptr};
  if (hasAttribute)
  {
    auto& selectorMap = m_cssSelectors[WebvttSelector::ATTRIBUTE];
    auto itCssStyle = selectorMap.find(tag.m_tag + " " + tag.m_annotation);
    if (itCssStyle != selectorMap.end())
      cssStyle = &itCssStyle->second;
  }
  else if (!tag.m_classes.empty())
  {
    auto& selectorMap = m_cssSelectors[WebvttSelector::CLASS];
    // Cascading classes not implemented
    const std::string className = "." + tag.m_classes[0];
    // Class selector that target a specific element have the priority
    auto itCssStyle = selectorMap.find(tag.m_tag + className);
    if (itCssStyle != selectorMap.end())
      cssStyle = &itCssStyle->second;

    if (!cssStyle)
    {
      auto itCssStyle = selectorMap.find(className);
      if (itCssStyle != selectorMap.end())
        cssStyle = &itCssStyle->second;
    }
  }
  else
  {
    auto& selectorMap = m_cssSelectors[WebvttSelector::TYPE];
    auto itCssStyle = selectorMap.find(tag.m_tag);
    if (itCssStyle != selectorMap.end())
      cssStyle = &itCssStyle->second;
  }

  if (cssStyle)
  {
    // Insert the CSS Style converted as tags
    const std::string tags = ConvertStyleToOpenTags(flagTags, *cssStyle);
    text.insert(pos, tags);
    pos += static_cast<int>(tags.length());
    // Keep track of the opened tags
    cssTagsOpened.emplace_front(tag.m_tag, cssStyle);
  }
}

void CWebVTTHandler::InsertCssStyleCloseTag(
    const tagToken& tag,
    std::string& text,
    int& pos,
    int flagTags[],
    std::deque<std::pair<std::string, webvttCssStyle*>>& cssTagsOpened,
    webvttCssStyle& baseStyle)
{
  if (cssTagsOpened.empty())
    return;

  std::pair<std::string, webvttCssStyle*> stylePair = cssTagsOpened.front();
  if (stylePair.first == tag.m_tag)
  {
    cssTagsOpened.pop_front();
    webvttCssStyle* style = &baseStyle;
    if (!cssTagsOpened.empty())
      style = cssTagsOpened.front().second;
    const std::string tags = ConvertStyleToCloseTags(flagTags, stylePair.second, style);
    text.insert(pos, tags);
    pos += static_cast<int>(tags.length());
  }
}

void CWebVTTHandler::ConvertAddSubtitle(std::vector<subtitleData>* subList)
{
  // Convert tags and apply the CSS Styles converted
  ConvertSubtitle(m_subtitleData.text);

  if (m_lastSubtitleData)
  {
    // Check for same subtitle data
    if (m_lastSubtitleData->startTime == m_subtitleData.startTime &&
        m_lastSubtitleData->stopTime == m_subtitleData.stopTime &&
        m_lastSubtitleData->textRaw == m_subtitleData.textRaw &&
        m_lastSubtitleData->cueSettings == m_subtitleData.cueSettings)
    {
      if (subList->empty())
      {
        // On segmented WebVTT, it can happen that the last subtitle entry is sent
        // on consecutive demux packet. Hence we avoid showing overlapping subs.
        return;
      }
      else
      {
        // Youtube WebVTT can have multiple cues with same time, text and position
        // sometimes with different css color but only last cue will be visible
        // this cause unexpected results on screen, so we keep only the current one
        // and delete the previous one.
        subList->pop_back();
      }
    }
  }

  subList->emplace_back(m_subtitleData);
  m_lastSubtitleData = std::make_unique<subtitleData>(m_subtitleData);
}

void CWebVTTHandler::LoadColors()
{
  CGUIColorManager colorManager;
  colorManager.LoadColorsListFromXML(
      CSpecialProtocol::TranslatePathConvertCase("special://xbmc/system/colors.xml"), m_CSSColors,
      false);
  m_CSSColorsLoaded = true;
}

double CWebVTTHandler::GetTimeFromRegexTS(CRegExp& regex, int indexStart /* = 1 */)
{
  int sHours = 0;
  if (!regex.GetMatch(indexStart).empty())
    sHours = std::stoi(regex.GetMatch(indexStart));
  int sMinutes = std::stoi(regex.GetMatch(indexStart + 1));
  double sSeconds = std::stod(regex.GetMatch(indexStart + 2));
  return (static_cast<double>(sHours * 3600 + sMinutes * 60) + sSeconds) * DVD_TIME_BASE;
}
