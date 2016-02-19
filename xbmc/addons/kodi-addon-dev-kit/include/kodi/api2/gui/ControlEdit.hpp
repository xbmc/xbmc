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
  class CWindow;

  //============================================================================
  ///
  /// \defgroup CPP_V2_KodiAPI_GUI_CControlEdit
  /// \ingroup CPP_V2_KodiAPI_GUI
  /// @{
  /// @brief <b>Editable window text control used as an input  control  for  the
  /// osd keyboard and other input fields</b>
  ///
  /// The edit control  allows a user to  input text in Kodi. You can choose the
  /// font, size, colour, location and header of the text to be displayed.
  ///
  /// It has the header \ref ControlEdit.h "#include <kodi/api2/gui/ControlEdit.h>"
  /// be included to enjoy it.
  ///
  /// Here  you  find  the   needed  skin  part  for  a   \ref skin_Edit_control
  /// "edit control".
  ///
  /// @note The  call of the  control is only possible  from  the  corresponding
  /// window as its class and identification number is required.
  ///
  class CControlEdit
  {
  public:
    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CControlEdit(CWindow* window, int controlId);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief Destructor
    ///
    virtual ~CControlEdit();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible              If true visible, otherwise hidden
    ///
    void SetVisible(bool visible);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief Set's the control's enabled/disabled state
    ///
    /// @param[in] enabled              If true enabled, otherwise disabled
    ///
    void SetEnabled(bool enabled);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief To set the text string on edit control
    ///
    /// @param[in] label                Text to show
    ///
    void SetLabel(const std::string& label);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief Returns the text heading for this edit control.
    ///
    /// @return                         Heading text
    ///
    std::string GetLabel() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief Set's text heading for this edit control.
    ///
    /// @param[in] text                 string or unicode - text string.
    ///
    void SetText(const std::string& text);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief Returns the text value for this edit control.
    ///
    /// @return                         Text value of control
    ///
    std::string GetText() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief Set the cursor position on text.
    ///
    /// @param[in] iPosition            The position to set
    ///
    void SetCursorPosition(unsigned int iPosition);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief To get current cursor position on text field
    ///
    /// @return                         The current cursor position
    ///
    unsigned int GetCursorPosition();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlEdit
    /// @brief To set field input type which are defined on \ref InputType
    ///
    /// @param[in] type                 The   \ref InputType "Add-on input type"
    ///                                 to use
    /// @param[in] heading              The heading  text for  related  keyboard
    ///                                 dialog
    ///
    void SetInputType(AddonGUIInputType type, const std::string& heading);
    //--------------------------------------------------------------------------

    IMPL_GUI_EDIT_CONTROL;
  };
  /// @}

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
