/*
 *      Copyright (C) 2005-2013 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <algorithm>
#include <set>

#include "KeyboardLayout.h"
#include "guilib/LocalizeStrings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "InputCodingTableFactory.h"

CKeyboardLayout::CKeyboardLayout()
{
  m_codingtable = NULL;
}

CKeyboardLayout::~CKeyboardLayout()
{ }

bool CKeyboardLayout::Load(const TiXmlElement* element)
{
  const char* language = element->Attribute("language");
  if (language == NULL)
  {
    CLog::Log(LOGWARNING, "CKeyboardLayout: invalid \"language\" attribute");
    return false;
  }

  m_language = language;
  if (m_language.empty())
  {
    CLog::Log(LOGWARNING, "CKeyboardLayout: empty \"language\" attribute");
    return false;
  }

  const char* layout = element->Attribute("layout");
  if (layout == NULL)
  {
    CLog::Log(LOGWARNING, "CKeyboardLayout: invalid \"layout\" attribute");
    return false;
  }

  m_layout = layout;
  if (m_layout.empty())
  {
    CLog::Log(LOGWARNING, "CKeyboardLayout: empty \"layout\" attribute");
    return false;
  }

  const TiXmlElement *keyboard = element->FirstChildElement("keyboard");
  if (element->Attribute("codingtable"))
    m_codingtable = IInputCodingTablePtr(CInputCodingTableFactory::CreateCodingTable(element->Attribute("codingtable"), element));
  else
    m_codingtable = NULL;
  while (keyboard != NULL)
  {
    // parse modifiers keys
    std::set<unsigned int> modifierKeysSet;

    const char* strModifiers = keyboard->Attribute("modifiers");
    if (strModifiers != NULL)
    {
      std::string modifiers = strModifiers;
      StringUtils::ToLower(modifiers);

      std::vector<std::string> variants = StringUtils::Split(modifiers, ",");
      for (std::vector<std::string>::const_iterator itv = variants.begin(); itv != variants.end(); ++itv)
      {
        unsigned int iKeys = ModifierKeyNone;
        std::vector<std::string> keys = StringUtils::Split(*itv, "+");
        for (std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); ++it)
        {
          std::string strKey = *it;
          if (strKey == "shift")
            iKeys |= ModifierKeyShift;
          else if (strKey == "symbol")
            iKeys |= ModifierKeySymbol;
        }

        modifierKeysSet.insert(iKeys);
      }
    }

    // parse keyboard rows
    const TiXmlNode *row = keyboard->FirstChild("row");
    while (row != NULL)
    {
      if (!row->NoChildren())
      {
        std::string strRow = row->FirstChild()->ValueStr();
        std::vector<std::string> chars = BreakCharacters(strRow);
        if (!modifierKeysSet.empty())
        {
          for (std::set<unsigned int>::const_iterator it = modifierKeysSet.begin(); it != modifierKeysSet.end(); ++it)
            m_keyboards[*it].push_back(chars);
        }
        else
          m_keyboards[ModifierKeyNone].push_back(chars);
      }

      row = row->NextSibling();
    }

    keyboard = keyboard->NextSiblingElement();
  }

  if (m_keyboards.empty())
  {
    CLog::Log(LOGWARNING, "CKeyboardLayout: no keyboard layout found");
    return false;
  }

  return true;
}

std::string CKeyboardLayout::GetIdentifier() const
{
  return StringUtils::Format("%s %s", m_language.c_str(), m_layout.c_str());
}

std::string CKeyboardLayout::GetName() const
{
  return StringUtils::Format(g_localizeStrings.Get(311).c_str(), m_language.c_str(), m_layout.c_str());
}

std::string CKeyboardLayout::GetCharAt(unsigned int row, unsigned int column, unsigned int modifiers) const
{
  Keyboards::const_iterator mod = m_keyboards.find(modifiers);
  if (modifiers != ModifierKeyNone && mod != m_keyboards.end() && mod->second.empty())
  {
    // fallback to basic keyboard
    mod = m_keyboards.find(ModifierKeyNone);
  }

  if (mod != m_keyboards.end())
  {
    if (row < mod->second.size() && column < mod->second[row].size())
    {
      std::string ch = mod->second[row][column];
      if (ch != " ")
        return ch;
    }
  }

  return "";
}

std::vector<std::string> CKeyboardLayout::BreakCharacters(const std::string &chars)
{
  std::vector<std::string> result;
  // break into utf8 characters
  std::u32string chars32 = g_charsetConverter.utf8ToUtf32(chars);
  for (std::u32string::const_iterator it = chars32.begin(); it != chars32.end(); ++it)
  {
    std::u32string char32(1, *it);
    result.push_back(g_charsetConverter.utf32ToUtf8(char32));
  }

  return result;
}
