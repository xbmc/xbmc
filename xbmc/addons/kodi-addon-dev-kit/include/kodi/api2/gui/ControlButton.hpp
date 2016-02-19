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

#include <string>

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
  class CWindow;

  //============================================================================
  ///
  /// \defgroup CPP_V2_KodiAPI_GUI_CControlButton
  /// \ingroup CPP_V2_KodiAPI_GUI
  /// @{
  /// @brief <b>Standard push button control for window</b>
  ///
  /// The  button  control  is used for creating push buttons  in Kodi.  You can
  /// choose the position,  size,  and look of the button,  as well as  choosing
  /// what action(s) should be performed when pushed.
  ///
  /// It has the header \ref ControlButton.h "#include <kodi/api2/gui/ControlButton.h>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref skin_Button_control "button control"
  ///
  /// @note The call of the control is  only  possible  from  the  corresponding
  /// window as its class and identification number is required.
  ///
  class CControlButton
  {
  public:
    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_GUI_CControlButton
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CControlButton(CWindow* window, int controlId);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_GUI_CControlButton
    /// @brief Destructor
    ///
    virtual ~CControlButton();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_GUI_CControlButton
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible              If true visible, otherwise hidden
    ///
    void SetVisible(bool visible);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_GUI_CControlButton
    /// @brief Set's the control's enabled/disabled state
    ///
    /// @param[in] enabled              If true enabled, otherwise disabled
    ///
    void SetEnabled(bool enabled);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_GUI_CControlButton
    /// @brief To set the text string on button
    ///
    /// @param[in] text                 Text to show
    ///
    void SetLabel(const std::string& text);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_GUI_CControlButton
    /// @brief Get the used text from button
    ///
    /// @return                         Text shown
    ///
    std::string GetLabel() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_GUI_CControlButton
    /// @brief If two labels are used for button becomes it set with them
    ///
    /// @param[in] text                 Text for second label
    ///
    void SetLabel2(const std::string& text);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_GUI_CControlButton
    /// @brief Get the second label if present
    ///
    /// @return                         Second label
    ///
    std::string GetLabel2() const;
    //--------------------------------------------------------------------------

    IMPL_GUI_BUTTON_CONTROL;
  };
  /// @}

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
