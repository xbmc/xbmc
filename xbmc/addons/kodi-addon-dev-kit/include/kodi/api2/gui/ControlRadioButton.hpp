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

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{
  class CWindow;

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_GUI_CControlRadioButton Control Radio Button
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>Window control for a radio button (as used for on/off settings)</b>
  ///
  /// The radio  button control is used for creating push button on/off settings
  /// in Kodi. You can choose the position,  size,  and look of the button. When
  /// the user clicks on the radio button,  the state will change,  toggling the
  /// extra  textures  (textureradioon and textureradiooff).  Used  for settings
  /// controls.
  ///
  /// It has the header \ref ControlRadioButton.hpp "#include <kodi/api2/gui/ControlRadioButton.hpp>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref Radio_button_control "radio button control"
  ///
  /// @note The  call  of the  control is  only possible  from the corresponding
  /// window as its class and identification number is required.
  ///
  class CControlRadioButton
  {
  public:
    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlRadioButton
    /// @brief Construct a new control
    ///
    /// @param[in] window     related window control class
    /// @param[in] controlId  Used skin xml control id
    ///
    CControlRadioButton(CWindow* window, int controlId);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CControlRadioButton
    /// @brief Destructor
    ///
    virtual ~CControlRadioButton();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlRadioButton
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible    If true visible, otherwise hidden
    ///
    void SetVisible(bool visible);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlRadioButton
    /// @brief Set's the control's enabled/disabled state
    ///
    /// @param[in] enabled    If true enabled, otherwise disabled
    ///
    void SetEnabled(bool enabled);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlRadioButton
    /// @brief To set the text string on radio button
    ///
    /// @param[in] label      Text to show
    ///
    void SetLabel(const std::string& label);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlRadioButton
    /// @brief Get the used text from control
    ///
    /// @return               Text shown
    ///
    std::string GetLabel() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlRadioButton
    /// @brief To set radio button condition to on or off
    ///
    /// @param[in] selected   true set radio button to  selection on,  otherwise
    ///                       off
    ///
    void SetSelected(bool selected);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlRadioButton
    /// @brief Get the current selected condition of radio button
    ///
    /// @return               Selected condition
    ///
    bool IsSelected() const;
    //--------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_GUI_RADIO_BUTTON_CONTROL;
  #endif
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
