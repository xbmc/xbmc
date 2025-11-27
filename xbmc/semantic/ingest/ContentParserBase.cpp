/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContentParserBase.h"

#include "utils/CharsetConverter.h"
#include "utils/HTMLUtil.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>

using namespace KODI::SEMANTIC;

void ContentParserBase::StripHTMLTags(std::string& text)
{
  if (text.empty())
    return;

  // Remove HTML tags using the existing Kodi utility
  HTML::CHTMLUtil::RemoveTags(text);

  // Convert HTML entities (like &nbsp;, &lt;, etc.)
  std::wstring wStrHtml, wStr;
  if (g_charsetConverter.utf8ToW(text, wStrHtml, false))
  {
    HTML::CHTMLUtil::ConvertHTMLToW(wStrHtml, wStr);
    g_charsetConverter.wToUTF8(wStr, text);
  }
}

void ContentParserBase::NormalizeWhitespace(std::string& text)
{
  if (text.empty())
    return;

  // Remove CRLF and normalize to LF
  StringUtils::RemoveCRLF(text);

  // Trim leading and trailing whitespace
  StringUtils::Trim(text);

  // Remove duplicate spaces and tabs
  StringUtils::RemoveDuplicatedSpacesAndTabs(text);
}

bool ContentParserBase::DetectCharset(std::string& content)
{
  if (content.empty())
    return true;

  // Check for UTF-8 BOM
  if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
      static_cast<unsigned char>(content[1]) == 0xBB &&
      static_cast<unsigned char>(content[2]) == 0xBF)
  {
    // Remove UTF-8 BOM
    content.erase(0, 3);
    return true;
  }

  // Check for UTF-16 LE BOM
  if (content.size() >= 2 && static_cast<unsigned char>(content[0]) == 0xFF &&
      static_cast<unsigned char>(content[1]) == 0xFE)
  {
    // Convert UTF-16 LE to UTF-8
    std::u16string utf16str(reinterpret_cast<const char16_t*>(content.data() + 2),
                            (content.size() - 2) / 2);
    std::string utf8str;
    if (CCharsetConverter::utf16LEtoUTF8(utf16str, utf8str))
    {
      content = utf8str;
      return true;
    }
    return false;
  }

  // Check for UTF-16 BE BOM
  if (content.size() >= 2 && static_cast<unsigned char>(content[0]) == 0xFE &&
      static_cast<unsigned char>(content[1]) == 0xFF)
  {
    // Convert UTF-16 BE to UTF-8
    std::u16string utf16str(reinterpret_cast<const char16_t*>(content.data() + 2),
                            (content.size() - 2) / 2);
    std::string utf8str;
    if (CCharsetConverter::utf16BEtoUTF8(utf16str, utf8str))
    {
      content = utf8str;
      return true;
    }
    return false;
  }

  // Try to detect charset and convert to UTF-8
  // This will handle Latin-1 and other common subtitle encodings
  std::string originalContent = content;
  if (CCharsetConverter::unknownToUTF8(content))
  {
    return true;
  }

  // If conversion failed, restore original and assume it's already UTF-8
  content = originalContent;
  return true;
}

bool ContentParserBase::IsNonDialogue(const std::string& text)
{
  if (text.empty())
    return true;

  std::string lowerText = text;
  StringUtils::ToLower(lowerText);
  StringUtils::Trim(lowerText);

  // Check for empty or whitespace-only
  if (lowerText.empty())
    return true;

  // Check for music indicators
  if (lowerText.find("♪") != std::string::npos || lowerText.find("♫") != std::string::npos)
    return true;

  // Check for common non-dialogue patterns in square brackets
  static const char* nonDialoguePatterns[] = {
      "[music]",      "[Music]",      "[MUSIC]",      "[music playing]",
      "[laughs]",     "[Laughs]",     "[LAUGHS]",     "[laughter]",
      "[applause]",   "[Applause]",   "[APPLAUSE]",   "[cheering]",
      "[sighs]",      "[Sighs]",      "[SIGHS]",      "[groans]",
      "[silence]",    "[Silence]",    "[SILENCE]",    "[static]",
      "[inaudible]",  "[Inaudible]",  "[INAUDIBLE]",  "[unintelligible]",
      "[no audio]",   "[No Audio]",   "[NO AUDIO]",   "[sound effect]",
      "[sound effects]"};

  for (const char* pattern : nonDialoguePatterns)
  {
    if (lowerText.find(StringUtils::ToLower(pattern)) != std::string::npos)
      return true;
  }

  // Check for generic sound effects pattern: [any text in brackets]
  // but only if the entire text is just the bracketed content
  if (lowerText.front() == '[' && lowerText.back() == ']')
  {
    std::string bracketed = lowerText.substr(1, lowerText.length() - 2);
    StringUtils::Trim(bracketed);
    // If it's just a short bracketed phrase, likely a sound effect
    if (bracketed.length() < 50 && bracketed.find('.') == std::string::npos)
      return true;
  }

  // Check for parenthetical sound effects: (door opens), (phone rings), etc.
  if (lowerText.front() == '(' && lowerText.back() == ')')
  {
    std::string parenthetical = lowerText.substr(1, lowerText.length() - 2);
    StringUtils::Trim(parenthetical);
    // Common sound effect verbs
    static const char* soundVerbs[] = {"rings",  "opens",  "closes", "slams",  "crashes",
                                       "beeps",  "buzzes", "clicks", "knocks", "creaks",
                                       "squeaks"};
    for (const char* verb : soundVerbs)
    {
      if (parenthetical.find(verb) != std::string::npos)
        return true;
    }
  }

  return false;
}

bool ContentParserBase::HasSupportedExtension(const std::string& path,
                                              const std::vector<std::string>& extensions)
{
  std::string ext = URIUtils::GetExtension(path);
  if (!ext.empty() && ext[0] == '.')
    ext = ext.substr(1); // Remove leading dot

  std::string lowerExt = ext;
  StringUtils::ToLower(lowerExt);

  return std::any_of(extensions.begin(), extensions.end(),
                     [&lowerExt](const std::string& supportedExt)
                     { return StringUtils::EqualsNoCase(lowerExt, supportedExt); });
}
