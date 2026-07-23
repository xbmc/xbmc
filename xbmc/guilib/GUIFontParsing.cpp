/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontParsing.h"

#include "GUIControlFactory.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <string>
#include <vector>

int ParseFontStyle(const TiXmlNode* fontNode)
{
  int iStyle = FONT_STYLE_NORMAL;
  std::string style;
  if (XMLUtils::GetString(fontNode, "style", style))
  {
    std::vector<std::string> styles = StringUtils::Tokenize(style, " ");
    for (const std::string& i : styles)
    {
      if (i == "bold")
        iStyle |= FONT_STYLE_BOLD;
      else if (i == "italics")
        iStyle |= FONT_STYLE_ITALICS;
      else if (i == "bolditalics") // backward compatibility
        iStyle |= (FONT_STYLE_BOLD | FONT_STYLE_ITALICS);
      else if (i == "uppercase")
        iStyle |= FONT_STYLE_UPPERCASE;
      else if (i == "lowercase")
        iStyle |= FONT_STYLE_LOWERCASE;
      else if (i == "capitalize")
        iStyle |= FONT_STYLE_CAPITALIZE;
      else if (i == "lighten")
        iStyle |= FONT_STYLE_LIGHT;
    }
  }

  return iStyle;
}

std::vector<FontDefinition> ParseFontSet(const TiXmlNode* firstFontNode)
{
  std::vector<FontDefinition> definitions;

  for (const TiXmlNode* fontNode = firstFontNode; fontNode;
       fontNode = fontNode->NextSibling("font"))
  {
    FontDefinition def;

    XMLUtils::GetString(fontNode, "name", def.name);
    XMLUtils::GetInt(fontNode, "size", def.size);
    XMLUtils::GetFloat(fontNode, "linespacing", def.lineSpacing);
    XMLUtils::GetFloat(fontNode, "aspect", def.aspect);
    CGUIControlFactory::GetColor(fontNode, "shadow", def.shadowColor);
    CGUIControlFactory::GetColor(fontNode, "color", def.textColor);
    XMLUtils::GetString(fontNode, "filename", def.fileName);
    def.style = ParseFontStyle(fontNode);

    // An empty <filename> is allowed through: an addon font scope reads it as
    // "inherit the active skin's typeface". The skin's own loader rejects it,
    // since there is nothing for a skin to inherit from.
    if (def.name.empty() ||
        (!def.fileName.empty() && !URIUtils::HasExtension(def.fileName, ".ttf")))
    {
      CLog::LogF(LOGDEBUG, "Skipping font entry, name '{}' filename '{}'", EscapeFontName(def.name),
                 EscapeFontName(def.fileName));
      continue;
    }

    definitions.emplace_back(std::move(def));
  }

  return definitions;
}

int StripIncludes(TiXmlElement* rootElement)
{
  if (!rootElement)
    return 0;

  int removed = 0;
  TiXmlElement* include = rootElement->FirstChildElement("include");
  while (include)
  {
    TiXmlElement* next = include->NextSiblingElement("include");
    CLog::LogF(LOGWARNING, "Ignoring <include> in addon font definition");
    rootElement->RemoveChild(include);
    ++removed;
    include = next;
  }

  return removed;
}

std::string EscapeFontName(const std::string& name)
{
  constexpr size_t MAX_LOGGED_NAME = 128;

  std::string escaped;
  escaped.reserve(name.size());

  for (const char c : name)
  {
    switch (c)
    {
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += c;
        break;
    }

    if (escaped.size() >= MAX_LOGGED_NAME)
      break;
  }

  if (escaped.size() > MAX_LOGGED_NAME)
    escaped.resize(MAX_LOGGED_NAME);

  return escaped;
}

bool IsAddonSafeFontFilename(const std::string& fileName)
{
  if (fileName.empty())
    return false;

  if (CURL::IsFullPath(fileName))
    return false;

  if (fileName.find("..") != std::string::npos)
    return false;

  return true;
}
