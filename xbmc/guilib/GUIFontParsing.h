/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFont.h"
#include "utils/ColorUtils.h"

#include <string>
#include <vector>

class TiXmlElement;
class TiXmlNode;

struct FontDefinition
{
  std::string name;
  std::string fileName;
  int size{20};
  float aspect{1.0f};
  float lineSpacing{1.0f};
  KODI::UTILS::COLOR::Color shadowColor{0};
  KODI::UTILS::COLOR::Color textColor{0};
  int style{FONT_STYLE_NORMAL};
};

/*!
 \brief Parse the <style> element of a <font> node into a FONT_STYLE_ bitmask.
 */
int ParseFontStyle(const TiXmlNode* fontNode);

/*!
 \brief Parse a chain of <font> siblings into definitions.
 \param firstFontNode the first <font> child of a <fontset>, or nullptr
 \return one definition per usable <font>. Entries with an empty <name> or a
         <filename> that is not a .ttf are skipped.

 Free of GL, CWinSystemBase and CGraphicContext. However, parsing a <color> or
 <shadow> element resolves the name through
 CServiceBroker::GetGUI()->GetColorManager(), so a CGUIComponent must be
 registered when the input contains such an element. The caller feeds each
 definition to LoadTTF.
 */
std::vector<FontDefinition> ParseFontSet(const TiXmlNode* firstFontNode);

/*!
 \brief Remove every <include> child of an addon-authored document root.
 \return the number of nodes removed

 An addon Font.xml must never reach the active skin's ResolveIncludes:
 <include file="resource://..."> loads addon-referenced XML into skin-global
 include state and <include condition="..."> registers an addon expression
 into the global InfoManager. Both cross a trust boundary.
 */
int StripIncludes(TiXmlElement* rootElement);

/*!
 \brief Make an addon-controlled font name safe to interpolate into a log line.
 Escapes CR, LF and TAB so a name cannot forge or split a log record, and
 truncates to 128 characters.
 */
std::string EscapeFontName(const std::string& name);
