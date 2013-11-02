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
     * Keyboard class.\n
     * \n
     * Keyboard([default, heading, hidden]) -- Creates a new Keyboard object with default text\n
     *                                 heading and hidden input flag if supplied.\n
     * \n
     * default        : [opt] string - default text entry.\n
     * heading        : [opt] string - keyboard heading.\n
     * hidden         : [opt] boolean - True for hidden text entry.\n
     * \n
     * example:\n
     *   - kb = xbmc.Keyboard('default', 'heading', True)
     *   - kb.setDefault('password') # optional
     *   - kb.setHeading('Enter password') # optional
     *   - kb.setHiddenInput(True) # optional
     *   - kb.doModal()
     *   - if (kb.isConfirmed()):
     *   -   text = kb.getText()
     */
    class Keyboard : public AddonClass
    {
    public:
#ifndef SWIG
      String strDefault;
      String strHeading;
      bool bHidden;
      String strText;
      bool bConfirmed;
#endif

      Keyboard(const String& line = emptyString, const String& heading = emptyString, bool hidden = false);
      virtual ~Keyboard();

      /**
       * doModal([autoclose]) -- Show keyboard and wait for user action.\n
       * \n
       * autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)\n
       * \n
       * example:
       *   - kb.doModal(30000)
       */
      void doModal(int autoclose = 0);

      // setDefault() Method
      /**
       * setDefault(default) -- Set the default text entry.\n
       * \n
       * default        : string - default text entry.\n
       * \n
       * example:
       *   - kb.setDefault('password')
       */
      void setDefault(const String& line = emptyString);

      /**
       * setHiddenInput(hidden) -- Allows hidden text entry.\n
       * \n
       * hidden        : boolean - True for hidden text entry.\n
       * example:
       *   - kb.setHiddenInput(True)
       */
      void setHiddenInput(bool hidden = false);

      // setHeading() Method
      /**
       * setHeading(heading) -- Set the keyboard heading.\n
       * \n
       * heading        : string - keyboard heading.\n
       * \n
       * example:
       *   - kb.setHeading('Enter password')
       */
      void setHeading(const String& heading);

      // getText() Method
      /**
       * getText() -- Returns the user input as a string.\n
       * \n
       * *Note, This will always return the text entry even if you cancel the keyboard.\n
       *        Use the isConfirmed() method to check if user cancelled the keyboard.\n
       * \n
       * example:
       *   - text = kb.getText()
       */
      String getText();

      // isConfirmed() Method
      /**
       * isConfirmed() -- Returns False if the user cancelled the input.\n
       * \n
       * example:
       *   - if (kb.isConfirmed()):
       */
      bool isConfirmed();
    };
  }
}
