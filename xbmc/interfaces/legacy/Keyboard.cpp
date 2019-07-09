/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Keyboard.h"

#include "LanguageHook.h"
#include "guilib/GUIKeyboardFactory.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/Variant.h"

using namespace KODI::MESSAGING;

namespace XBMCAddon
{
  namespace xbmc
  {

    Keyboard::Keyboard(const String& line /* = nullString*/, const String& heading/* = nullString*/, bool hidden/* = false*/)
      : strDefault(line), strHeading(heading), bHidden(hidden)
    {
    }

    Keyboard::~Keyboard() = default;

    void Keyboard::doModal(int autoclose)
    {
      DelayedCallGuard dg(languageHook);
      // using keyboardfactory method to get native keyboard if there is.
      strText = strDefault;
      std::string text(strDefault);
      bConfirmed = CGUIKeyboardFactory::ShowAndGetInput(text, CVariant{strHeading}, true, bHidden, autoclose * 1000);
      strText = text;
    }

    void Keyboard::setDefault(const String& line)
    {
      strDefault = line;
    }

    void Keyboard::setHiddenInput(bool hidden)
    {
      bHidden = hidden;
    }

    void Keyboard::setHeading(const String& heading)
    {
      strHeading = heading;
    }

    String Keyboard::getText()
    {
      return strText;
    }

    bool Keyboard::isConfirmed()
    {
      return bConfirmed;
    }
  }
}

