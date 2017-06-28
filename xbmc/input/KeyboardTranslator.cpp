/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "KeyboardTranslator.h"
#include "Key.h"
#include "XBMC_keytable.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <string>
#include <vector>

uint32_t CKeyboardTranslator::TranslateButton(const TiXmlElement *pButton)
{
  uint32_t button_id = 0;
  const char *szButton = pButton->Value();

  if (szButton == nullptr)
    return 0;

  const std::string strKey = szButton;
  if (strKey == "key")
  {
    std::string strID;
    if (pButton->QueryValueAttribute("id", &strID) == TIXML_SUCCESS)
    {
      const char *str = strID.c_str();
      char *endptr;
      long int id = strtol(str, &endptr, 0);
      if (endptr - str != (int)strlen(str) || id <= 0 || id > 0x00FFFFFF)
        CLog::Log(LOGDEBUG, "%s - invalid key id %s", __FUNCTION__, strID.c_str());
      else
        button_id = (uint32_t)id;
    }
    else
      CLog::Log(LOGERROR, "Keyboard Translator: `key' button has no id");
  }
  else
    button_id = TranslateString(szButton);

  // Process the ctrl/shift/alt modifiers
  std::string strMod;
  if (pButton->QueryValueAttribute("mod", &strMod) == TIXML_SUCCESS)
  {
    StringUtils::ToLower(strMod);

    std::vector<std::string> modArray = StringUtils::Split(strMod, ",");
    for (auto substr : modArray)
    {
      StringUtils::Trim(substr);

      if (substr == "ctrl" || substr == "control")
        button_id |= CKey::MODIFIER_CTRL;
      else if (substr == "shift")
        button_id |= CKey::MODIFIER_SHIFT;
      else if (substr == "alt")
        button_id |= CKey::MODIFIER_ALT;
      else if (substr == "super" || substr == "win")
        button_id |= CKey::MODIFIER_SUPER;
      else if (substr == "meta" || substr == "cmd")
        button_id |= CKey::MODIFIER_META;
      else if (substr == "longpress")
        button_id |= CKey::MODIFIER_LONG;
      else
        CLog::Log(LOGERROR, "Keyboard Translator: Unknown key modifier %s in %s", substr.c_str(), strMod.c_str());
    }
  }

  return button_id;
}

uint32_t CKeyboardTranslator::TranslateString(const std::string &szButton)
{
  uint32_t buttonCode = 0;
  XBMCKEYTABLE keytable;

  // Look up the key name
  if (KeyTableLookupName(szButton, &keytable))
  {
    buttonCode = keytable.vkey;
  }
  else
  {
    // The lookup failed i.e. the key name wasn't found
    CLog::Log(LOGERROR, "Keyboard Translator: Can't find button %s", szButton.c_str());
  }

  buttonCode |= KEY_VKEY;

  return buttonCode;
}
