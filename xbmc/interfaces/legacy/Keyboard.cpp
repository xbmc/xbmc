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

#include "Keyboard.h"
#include "LanguageHook.h"

#include "guilib/GUIKeyboardFactory.h"

namespace XBMCAddon
{
  namespace xbmc
  {

    Keyboard::Keyboard(const String& line /* = nullString*/, const String& heading/* = nullString*/, bool hidden/* = false*/) 
      : strDefault(line), strHeading(heading), bHidden(hidden), bConfirmed(false)
    {
    }

    Keyboard::~Keyboard() {}

    void Keyboard::doModal(int autoclose)
    {
      DelayedCallGuard dg(languageHook);
      // using keyboardfactory method to get native keyboard if there is.
      strText = strDefault;
      std::string text(strDefault);
      bConfirmed = CGUIKeyboardFactory::ShowAndGetInput(text, strHeading, true, bHidden, autoclose * 1000);
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

