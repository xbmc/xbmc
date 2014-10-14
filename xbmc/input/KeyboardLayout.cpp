/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "KeyboardLayout.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/CharsetConverter.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include <set>

#define KEYBOARD_LAYOUTS_XML "special://xbmc/system/keyboardlayouts.xml"

CKeyboardLayout::CKeyboardLayout() : m_locale(""), m_variant("")
{
}

CKeyboardLayout::CKeyboardLayout(const std::string &localeName, const std::string &variantName, const TiXmlElement &element)
  : m_locale(localeName), m_variant(variantName)
{
  const TiXmlElement *keyboard = element.FirstChildElement("keyboard");
  while (keyboard)
  {
    // parse modifiers keys
    std::set<unsigned int> modifierKeysSet;

    if (keyboard->Attribute("modifiers"))
    {
      std::string modifiers = keyboard->Attribute("modifiers");
      StringUtils::ToLower(modifiers);

      std::vector<std::string> variants = StringUtils::Split(modifiers, ",");
      for (std::vector<std::string>::const_iterator itv = variants.begin(); itv != variants.end(); itv++)
      {
        unsigned int iKeys = MODIFIER_KEY_NONE;
        std::vector<std::string> keys = StringUtils::Split(*itv, "+");
        for (std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); it++)
        {
          std::string strKey = *it;
          if (strKey == "shift")
            iKeys |= MODIFIER_KEY_SHIFT;
          else if (strKey == "symbol")
            iKeys |= MODIFIER_KEY_SYMBOL;
        }
        modifierKeysSet.insert(iKeys);
      }
    }

    // parse keyboard rows
    const TiXmlNode *row = keyboard->FirstChild("row");
    while (row)
    {
      if (!row->NoChildren())
      {
        std::string strRow = row->FirstChild()->ValueStr();
        std::vector<std::string> chars = BreakCharacters(strRow);
        if (!modifierKeysSet.empty())
        {
          for (std::set<unsigned int>::const_iterator it = modifierKeysSet.begin(); it != modifierKeysSet.end(); it++)
            m_keyboards[*it].push_back(chars);
        }
        else
          m_keyboards[MODIFIER_KEY_NONE].push_back(chars);
      }
      row = row->NextSibling();
    }

    keyboard = keyboard->NextSiblingElement();
  }
}

CKeyboardLayout::~CKeyboardLayout(void)
{

}

std::string CKeyboardLayout::GetName() const
{
  std::string languageName;
  if (!g_LangCodeExpander.Lookup(languageName, GetLocale()))
    languageName = GetLocale();
  return StringUtils::Format("%s %s", languageName.c_str(), GetVariant().c_str());
}

std::string CKeyboardLayout::GetCharAt(unsigned int row, unsigned int column, unsigned int modifiers) const
{
  Keyboards::const_iterator mod = m_keyboards.find(modifiers);
  if (modifiers != MODIFIER_KEY_NONE && mod != m_keyboards.end() && mod->second.empty())
  {
    // fallback to basic keyboard
    mod = m_keyboards.find(MODIFIER_KEY_NONE);
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
  for (size_t i = 0; i < chars32.size(); i++)
  {
    std::u32string char32(1, chars32[i]);
    result.push_back(g_charsetConverter.utf32ToUtf8(char32));
  }
  return result;
}

std::vector<CKeyboardLayout> CKeyboardLayout::LoadLayouts()
{
  std::vector<CKeyboardLayout> result;
  CXBMCTinyXML xmlDoc;
  if (xmlDoc.LoadFile(KEYBOARD_LAYOUTS_XML))
  {
    const TiXmlElement* root = xmlDoc.RootElement();
    if (root && root->ValueStr() == "keyboardlayouts")
    {
      const TiXmlElement* layout = root->FirstChildElement("layout");
      while (layout)
      {
        const char* locale = layout->Attribute("locale");
        if (locale)
        {
          const char* variant = layout->Attribute("variant");
          std::string variantName = variant != NULL ? variant : "";

          result.push_back(CKeyboardLayout(locale, variantName, *layout));
        }
        layout = layout->NextSiblingElement("layout");
      }
    }
  }
  return result;
}

std::vector<CKeyboardLayout> CKeyboardLayout::LoadActiveLayouts()
{
  std::vector<CKeyboardLayout> allLayouts = LoadLayouts();

  const CSetting *setting = CSettings::Get().GetSetting("locale.keyboardlayouts");
  std::vector<std::string> layoutIds;
  if (setting)
    layoutIds = StringUtils::Split(setting->ToString(), '|');

  std::vector<CKeyboardLayout> result;
  for (std::vector<CKeyboardLayout>::const_iterator it = allLayouts.begin(); it != allLayouts.end(); ++it)
  {
    std::string layoutId = it->GetLocale() + "_" + it->GetVariant();
    if (std::find(layoutIds.begin(), layoutIds.end(), layoutId) != layoutIds.end())
      result.push_back(*it);
  }
  return result;
}


void CKeyboardLayout::SettingOptionsKeyboardLayoutsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  std::vector<CKeyboardLayout> layouts = LoadLayouts();
  for (std::vector<CKeyboardLayout>::const_iterator it = layouts.begin(); it != layouts.end(); it++)
  {
    std::string name = it->GetName();
    std::string settingId = it->GetLocale() + "_" + it->GetVariant();
    list.push_back(make_pair(name, settingId));
  }
  std::sort(list.begin(), list.end());
}
