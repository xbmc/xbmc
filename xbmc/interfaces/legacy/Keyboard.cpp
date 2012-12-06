/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Keyboard.h"
#include "LanguageHook.h"

#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "ApplicationMessenger.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    Keyboard::Keyboard(const String& line /* = nullString*/, const String& heading/* = nullString*/, bool hidden/* = false*/) 
      : AddonClass("Keyboard"), strDefault(line), strHeading(heading), bHidden(hidden), dlg(NULL) 
    {
      dlg = (CGUIDialogKeyboardGeneric*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
    }

    Keyboard::~Keyboard() {}

    void Keyboard::doModal(int autoclose) throw (KeyboardException)
    {
      DelayedCallGuard dg(languageHook);
      CGUIDialogKeyboardGeneric *pKeyboard = dlg;
      if(!pKeyboard)
        throw KeyboardException("Unable to load virtual keyboard");

      pKeyboard->Initialize();
      pKeyboard->SetHeading(strHeading);
      pKeyboard->SetText(strDefault);
      pKeyboard->SetHiddenInput(bHidden);
      if (autoclose > 0)
        pKeyboard->SetAutoClose(autoclose);

      // do modal of dialog
      ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, (DWORD)g_windowManager.GetActiveWindow()};
      CApplicationMessenger::Get().SendMessage(tMsg, true);
    }

    void Keyboard::setDefault(const String& line) throw (KeyboardException)
    {
      strDefault = line;

      CGUIDialogKeyboardGeneric *pKeyboard = dlg;
      if(!pKeyboard)
        throw KeyboardException("Unable to load keyboard");

      pKeyboard->SetText(strDefault);
    }

    void Keyboard::setHiddenInput(bool hidden) throw (KeyboardException)
    {
      bHidden = hidden;

      CGUIDialogKeyboardGeneric *pKeyboard = dlg;
      if(!pKeyboard)
        throw KeyboardException("Unable to load keyboard");

      pKeyboard->SetHiddenInput(bHidden);
    }

    void Keyboard::setHeading(const String& heading) throw (KeyboardException)
    {
      strHeading = heading;

      CGUIDialogKeyboardGeneric *pKeyboard = dlg;
      if(!pKeyboard)
        throw KeyboardException("Unable to load keyboard");

      pKeyboard->SetHeading(strHeading);
    }

    String Keyboard::getText() throw (KeyboardException)
    {
      CGUIDialogKeyboardGeneric *pKeyboard = dlg;
      if(!pKeyboard)
        throw KeyboardException("Unable to load keyboard");
      return pKeyboard->GetText();
    }

    bool Keyboard::isConfirmed() throw (KeyboardException)
    {
      CGUIDialogKeyboardGeneric *pKeyboard = dlg;
      if(!pKeyboard)
        throw KeyboardException("Unable to load keyboard");
      return pKeyboard->IsConfirmed();
    }
  }
}

