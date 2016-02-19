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
     * \defgroup python_keyboard Keyboard
     * \ingroup python_xbmc
     * @{
     * @brief <b>Kodi's keyboard class.</b>
     *
     * <b><c>xbmc.Keyboard([default, heading, hidden])</c></b>
     *
     * Creates a new Keyboard object with default text
     * heading and hidden input flag if supplied.
     *
     * @param default    : [opt] string - default text entry.
     * @param heading    : [opt] string - keyboard heading.
     * @param hidden     : [opt] boolean - True for hidden text entry.
     *
     *
     *--------------------------------------------------------------------------
     *
     * <b>Example:</b>
     * @code{.py}
     * ..
     * kb = xbmc.Keyboard('default', 'heading', True)
     * kb.setDefault('password') # optional
     * kb.setHeading('Enter password') # optional
     * kb.setHiddenInput(True) # optional
     * kb.doModal()
     * if (kb.isConfirmed()):
     *   text = kb.getText()
     * ..
     * @endcode
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
       * \ingroup python_keyboard
       * Show keyboard and wait for user action.
       *
       * @param autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * kb.doModal(30000)
       * ..
       * @endcode
       */
      void doModal(int autoclose = 0);

      // setDefault() Method
      /**
       * \ingroup python_keyboard
       * Set the default text entry.
       *
       * @param line        : string - default text entry.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * kb.setDefault('password')
       * ..
       * @endcode
       */
      void setDefault(const String& line = emptyString);

      /**
       * \ingroup python_keyboard
       * Allows hidden text entry.
       *
       * @param hidden        : boolean - True for hidden text entry.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * kb.setHiddenInput(True)
       * ..
       * @endcode
       */
      void setHiddenInput(bool hidden = false);

      // setHeading() Method
      /**
       * \ingroup python_keyboard
       * Set the keyboard heading.
       *
       * @param heading        : string - keyboard heading.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * kb.setHeading('Enter password')
       * ..
       * @endcode
       */
      void setHeading(const String& heading);

      // getText() Method
      /**
       * \ingroup python_keyboard
       * Returns the user input as a string.\n
       *
       * @note This will always return the text entry even if you cancel the keyboard.
       *       Use the isConfirmed() method to check if user cancelled the keyboard.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * text = kb.getText()
       * ..
       * @endcode
       */
      String getText();

      // isConfirmed() Method
      /**
       * \ingroup python_keyboard
       * Returns False if the user cancelled the input.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * if (kb.isConfirmed()):
       *   ..
       * @endcode
       */
      bool isConfirmed();
    };
    //@}
  }
}
