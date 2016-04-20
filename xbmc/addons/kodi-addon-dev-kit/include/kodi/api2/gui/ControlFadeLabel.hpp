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
  /// \defgroup CPP_KodiAPI_GUI_CControlFadeLabel Control Fade Label
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>Window control used to show multiple pieces of text  in the same
  /// position, by fading from one to the other</b>
  ///
  /// The fade label  control is used for displaying multiple pieces  of text in
  /// the same  space in  Kodi. You can choose  the font, size, colour, location
  /// and contents  of the text to be displayed.  The first piece of information
  /// to display fades in over 50 frames, then scrolls off to the left.  Once it
  /// is  finished scrolling off screen,  the second piece  of information fades
  /// in and  the process repeats.  A fade label  control is not  supported in a
  /// list container.
  ///
  /// It has the header \ref ControlFadeLabel.hpp "#include <kodi/api2/gui/ControlFadeLabel.hpp>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref Fade_Label_Control "fade label control"
  ///
  /// @note The  call of the  control is only  possible from  the  corresponding
  /// window as its class and identification number is required.
  ///
  class CControlFadeLabel
  {
  public:
    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlFadeLabel
    /// @brief Construct a new control.
    ///
    /// @param[in] window     related window control class
    /// @param[in] controlId  Used skin xml control id
    ///
    CControlFadeLabel(CWindow* window, int controlId);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlFadeLabel
    /// @brief Destructor.
    ///
    virtual ~CControlFadeLabel();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlFadeLabel
    /// @brief Set the control on window to visible.
    ///
    /// @param[in] visible    If true visible, otherwise hidden
    ///
    void SetVisible(bool visible);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlFadeLabel
    /// @brief To add additional text string on fade label.
    ///
    /// @param[in] label      Text to show
    ///
    void AddLabel(const std::string& label);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlFadeLabel
    /// @brief Get the used text from button
    ///
    /// @return               Text shown
    ///
    std::string GetLabel() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlFadeLabel
    /// @brief To enable or disable scrolling on fade label
    ///
    /// @param[in] scroll     To  enable scrolling  set  to true,  otherwise  is
    ///                       disabled
    ///
    void SetScrolling(bool scroll);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CControlFadeLabel
    /// @brief To reset al inserted labels.
    ///
    void Reset();

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_GUI_FADELABEL_CONTROL;
  #endif
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
