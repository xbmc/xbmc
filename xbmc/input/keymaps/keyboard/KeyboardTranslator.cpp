/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeyboardTranslator.h"

#include "input/keyboard/Key.h"
#include "input/keyboard/KeyIDs.h"
#include "input/keyboard/XBMC_keytable.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <string>
#include <vector>

#include <tinyxml2.h>

using namespace KODI;
using namespace KEYMAP;

uint32_t CKeyboardTranslator::TranslateButton(const tinyxml2::XMLElement* pButton)
{
  uint32_t button_id = 0;
  const char* szButton = pButton->Value();

  if (szButton == nullptr)
    return 0;

  const std::string strKey = szButton;
  if (strKey == "key")
  {
    const char* strID;
    if (pButton->QueryStringAttribute("id", &strID) == tinyxml2::XML_SUCCESS)
    {
      const char* str = strID;
      char* endptr;
      long int id = strtol(str, &endptr, 0);
      if (endptr - str != (int)strlen(str) || id <= 0 || id > 0x00FFFFFF)
        CLog::Log(LOGDEBUG, "{} - invalid key id {}", __FUNCTION__, strID);
      else
        button_id = (uint32_t)id;
    }
    else
      CLog::Log(LOGERROR, "Keyboard Translator: `key' button has no id");
  }
  else
    button_id = TranslateString(szButton);

  // Process the ctrl/shift/alt modifiers
  const char* strMod;
  if (pButton->QueryStringAttribute("mod", &strMod) == tinyxml2::XML_SUCCESS)
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
        CLog::Log(LOGERROR, "Keyboard Translator: Unknown key modifier {} in {}", substr, strMod);
    }
  }

  return button_id;
}

uint32_t CKeyboardTranslator::TranslateString(const std::string& szButton)
{
  uint32_t buttonCode = 0;
  KEYBOARD::XBMCKEYTABLE keytable;

  // Look up the key name
  if (KEYBOARD::KeyTable::LookupName(szButton, &keytable))
  {
    buttonCode = keytable.vkey;
  }
  else
  {
    // The lookup failed i.e. the key name wasn't found
    CLog::Log(LOGERROR, "Keyboard Translator: Can't find button {}", szButton);
  }

  buttonCode |= KEY_VKEY;

  return buttonCode;
}
