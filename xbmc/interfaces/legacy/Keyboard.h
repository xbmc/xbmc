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

#pragma once

#include "AddonClass.h"
#include "Exception.h"
#include "AddonString.h"

class CGUIDialogKeyboardGeneric;

namespace XBMCAddon
{
  namespace xbmc
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(KeyboardException);

    /**
     * <pre>
     * Keyboard class.
     * 
     * Keyboard([default, heading, hidden]) -- Creates a new Keyboard object with default text
     *                                 heading and hidden input flag if supplied.
     * 
     * default        : [opt] string - default text entry.
     * heading        : [opt] string - keyboard heading.
     * hidden         : [opt] boolean - True for hidden text entry.
     * 
     * example:
     *   - kb = xbmc.Keyboard('default', 'heading', True)
     *   - kb.setDefault('password') # optional
     *   - kb.setHeading('Enter password') # optional
     *   - kb.setHiddenInput(True) # optional
     *   - kb.doModal()
     *   - if (kb.isConfirmed()):
     *   -   text = kb.getText()
     * </pre>
     */
    class Keyboard : public AddonClass
    {
    public:
      String strDefault;
      String strHeading;
      bool bHidden;
      CGUIDialogKeyboardGeneric* dlg;

      Keyboard(const String& line = emptyString, const String& heading = emptyString, bool hidden = false);
      virtual ~Keyboard();

      /**
       * <pre>
       * doModal([autoclose]) -- Show keyboard and wait for user action.
       * 
       * autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       * 
       * example:
       *   - kb.doModal(30000)
       * </pre>
       */
      void doModal(int autoclose = 0) throw (KeyboardException);

      // setDefault() Method
      /**
       * <pre>
       * setDefault(default) -- Set the default text entry.
       * 
       * default        : string - default text entry.
       * 
       * example:
       *   - kb.setDefault('password')
       * </pre>
       */
      void setDefault(const String& line = emptyString) throw (KeyboardException);

      /**
       * <pre>
       * setHiddenInput(hidden) -- Allows hidden text entry.
       * 
       * hidden        : boolean - True for hidden text entry.
       * example:
       *   - kb.setHiddenInput(True)
       * </pre>
       */
      void setHiddenInput(bool hidden = false) throw (KeyboardException);

      // setHeading() Method
      /**
       * <pre>
       * setHeading(heading) -- Set the keyboard heading.
       * 
       * heading        : string - keyboard heading.
       * 
       * example:
       *   - kb.setHeading('Enter password')
       * </pre>
       */
      void setHeading(const String& heading) throw (KeyboardException);

      // getText() Method
      /**
       * <pre>
       * getText() -- Returns the user input as a string.
       * 
       * *Note, This will always return the text entry even if you cancel the keyboard.
       *        Use the isConfirmed() method to check if user cancelled the keyboard.
       * 
       * example:
       *   - text = kb.getText()
       * </pre>
       */
      String getText() throw (KeyboardException);

      // isConfirmed() Method
      /**
       * <pre>
       * isConfirmed() -- Returns False if the user cancelled the input.
       * 
       * example:
       *   - if (kb.isConfirmed()):
       * </pre>
       */
      bool isConfirmed() throw (KeyboardException);

    };
  }
}
