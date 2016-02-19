#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


namespace V2
{
namespace KodiAPI
{

namespace GUI
{

/*!

\defgroup CPP_V2_KodiAPI_GUI Library - GUILIB
\ingroup cpp
\brief <b><em>Graphical functions for Windows and Dialogs to show</em></b>

*/

  //============================================================================
  ///
  /// \defgroup CPP_V2_KodiAPI_GUI_General
  /// \ingroup CPP_V2_KodiAPI_GUI
  /// @{
  /// @brief <b>Allow use of binary classes and function to use on add-on's</b>
  ///
  /// Permits the use of the required functions of the add-on to Kodi. This class
  /// also contains some functions to the control.
  ///
  /// These are pure functions them no other initialization need.
  ///
  /// It has the header \ref General.h "#include <kodi/api2/gui/General.h>" be included
  /// to enjoy it.
  ///
  namespace General
  {
    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_General
    /// @brief Performs a graphical lock of rendering engine
    ///
    void Lock();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_General
    /// @brief Performs a graphical unlock of previous locked rendering engine
    ///
    void Unlock();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_General
    /// @brief Return the the current screen height with pixel
    ///
    int GetScreenHeight();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_General
    /// @brief Return the the current screen width with pixel
    ///
    int GetScreenWidth();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_General
    /// @brief Return the the current screen rendering resolution
    ///
    int GetVideoResolution();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_General
    /// @brief Returns the id for the current 'active' dialog as an integer.
    ///
    /// @return                        The currently active dialog Id
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// ..
    /// int wid = KodiAPI::GUI::General::GetCurrentWindowDialogId()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    int GetCurrentWindowDialogId();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_General
    /// @brief Returns the id for the current 'active' window as an integer.
    ///
    /// @return                        The currently active window Id
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// ..
    /// int wid = KodiAPI::GUI::General::GetCurrentWindowId()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    int GetCurrentWindowId();
    //--------------------------------------------------------------------------

  };
  /// @}

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
