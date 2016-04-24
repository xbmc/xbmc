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

    ///
    /// \defgroup python_keyboard Keyboard
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's keyboard class.**
    ///
    /// \python_class{ xbmc.Keyboard([default, heading, hidden]) }
    ///
    /// Creates a new Keyboard object with default text
    /// heading and hidden input flag if supplied.
    ///
    /// @param default    : [opt] string - default text entry.
    /// @param heading    : [opt] string - keyboard heading.
    /// @param hidden     : [opt] boolean - True for hidden text entry.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// kb = xbmc.Keyboard('default', 'heading', True)
    /// kb.setDefault('password') # optional
    /// kb.setHeading('Enter password') # optional
    /// kb.setHiddenInput(True) # optional
    /// kb.doModal()
    /// if (kb.isConfirmed()):
    ///   text = kb.getText()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
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

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_keyboard
      /// @brief \python_func{ doModal([autoclose]) }
      ///-----------------------------------------------------------------------
      /// Show keyboard and wait for user action.
      ///
      /// @param autoclose      [opt] integer - milliseconds to autoclose
      ///                       dialog. (default=do not autoclose)
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// kb.doModal(30000)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      doModal(...);
#else
      void doModal(int autoclose = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      // setDefault() Method
      ///
      /// \ingroup python_keyboard
      /// @brief \python_func{ setDefault(line) }
      ///-----------------------------------------------------------------------
      /// Set the default text entry.
      ///
      /// @param line        string - default text entry.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// kb.setDefault('password')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setDefault(...);
#else
      void setDefault(const String& line = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_keyboard
      /// @brief \python_func{ setHiddenInput(hidden) }
      ///-----------------------------------------------------------------------
      /// Allows hidden text entry.
      ///
      /// @param hidden        boolean - True for hidden text entry.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// kb.setHiddenInput(True)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setHiddenInput(...);
#else
      void setHiddenInput(bool hidden = false);
#endif

      // setHeading() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_keyboard
      /// @brief \python_func{ setHeading(heading) }
      ///-----------------------------------------------------------------------
      /// Set the keyboard heading.
      ///
      /// @param heading        string - keyboard heading.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// kb.setHeading('Enter password')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setHeading(...);
#else
      void setHeading(const String& heading);
#endif

      // getText() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_keyboard
      /// @brief \python_func{ getText() }
      ///-----------------------------------------------------------------------
      /// Returns the user input as a string.
      ///
      /// @note This will always return the text entry even if you cancel the keyboard.
      ///       Use the isConfirmed() method to check if user cancelled the keyboard.
      ///
      /// @return     get the in keyboard entered text
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// text = kb.getText()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getText();
#else
      String getText();
#endif

      // isConfirmed() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_keyboard
      /// @brief \python_func{ isConfirmed() }
      ///-----------------------------------------------------------------------
      /// Returns False if the user cancelled the input.
      ///
      /// @return     true if confirmed, if cancelled false
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// if (kb.isConfirmed()):
      ///   ..
      /// ~~~~~~~~~~~~~
      ///
      isConfirmed();
#else
      bool isConfirmed();
#endif
    };
    //@}
  }
}
