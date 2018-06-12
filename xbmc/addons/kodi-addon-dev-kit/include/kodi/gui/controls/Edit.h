/*
 *      Copyright (C) 2005-2017 Team KODI
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

#pragma once

#include "../../AddonBase.h"
#include "../Window.h"

namespace kodi
{
namespace gui
{
namespace controls
{

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_controls_CEdit Control Edit
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::controls::CEdit }
  /// **Editable window text control used as an input control for the osd keyboard
  /// and other input fields**
  ///
  /// The edit control  allows a user to  input text in Kodi. You can choose the
  /// font, size, colour, location and header of the text to be displayed.
  ///
  /// It has the header \ref Edit.h "#include <kodi/gui/controls/Edit.h>"
  /// be included to enjoy it.
  ///
  /// Here  you  find  the   needed  skin  part  for  a   \ref skin_Edit_control
  /// "edit control".
  ///
  /// @note The  call of the  control is only possible  from  the  corresponding
  /// window as its class and identification number is required.
  ///

  //============================================================================
  // see gui/definition.h for use of group "cpp_kodi_gui_controls_CEdit_Defs"
  ///
  /// \defgroup cpp_kodi_gui_controls_CEdit_Defs Definitions, structures and enumerators
  /// \ingroup cpp_kodi_gui_controls_CEdit
  /// @brief **Library definition values**
  ///

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */

//============================================================================
///
/// \ingroup cpp_kodi_gui_controls_CEdit_Defs
/// @{
/// @anchor AddonGUIInputType
/// @brief Text input types used on kodi::gui::controls::CEdit
enum AddonGUIInputType
{
  /// Text inside edit control only readable
  ADDON_INPUT_TYPE_READONLY = -1,
  /// Normal text entries
  ADDON_INPUT_TYPE_TEXT = 0,
  /// To use on edit control only numeric numbers
  ADDON_INPUT_TYPE_NUMBER,
  /// To insert seconds
  ADDON_INPUT_TYPE_SECONDS,
  /// To insert time
  ADDON_INPUT_TYPE_TIME,
  /// To insert a date
  ADDON_INPUT_TYPE_DATE,
  /// Used for write in IP addresses
  ADDON_INPUT_TYPE_IPADDRESS,
  /// Text field used as password entry field with not visible text
  ADDON_INPUT_TYPE_PASSWORD,
  /// Text field used as password entry field with not visible text but
  /// returned as MD5 value
  ADDON_INPUT_TYPE_PASSWORD_MD5,
  /// Use text field for search purpose
  ADDON_INPUT_TYPE_SEARCH,
  /// Text field as filter
  ADDON_INPUT_TYPE_FILTER,
  ///
  ADDON_INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW
};
/// @}
//----------------------------------------------------------------------------

namespace kodi
{
namespace gui
{
namespace controls
{

  class CEdit : public CAddonGUIControlBase
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CEdit(CWindow* window, int controlId)
      : CAddonGUIControlBase(window)
    {
      m_controlHandle = m_interface->kodi_gui->window->get_control_edit(m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
      if (!m_controlHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::control::CEdit can't create control class from Kodi !!!");
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief Destructor
    ///
    ~CEdit() override = default;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible              If true visible, otherwise hidden
    ///
    void SetVisible(bool visible)
    {
      m_interface->kodi_gui->control_edit->set_visible(m_interface->kodiBase, m_controlHandle, visible);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief Set's the control's enabled/disabled state
    ///
    /// @param[in] enabled              If true enabled, otherwise disabled
    ///
    void SetEnabled(bool enabled)
    {
      m_interface->kodi_gui->control_edit->set_enabled(m_interface->kodiBase, m_controlHandle, enabled);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief To set the text string on edit control
    ///
    /// @param[in] label                Text to show
    ///
    void SetLabel(const std::string& label)
    {
      m_interface->kodi_gui->control_edit->set_label(m_interface->kodiBase, m_controlHandle, label.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief Returns the text heading for this edit control.
    ///
    /// @return                         Heading text
    ///
    std::string GetLabel() const
    {
      std::string label;
      char* ret = m_interface->kodi_gui->control_edit->get_label(m_interface->kodiBase, m_controlHandle);
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          label = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return label;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief Set's text heading for this edit control.
    ///
    /// @param[in] text                 string or unicode - text string.
    ///
    void SetText(const std::string& text)
    {
      m_interface->kodi_gui->control_edit->set_text(m_interface->kodiBase, m_controlHandle, text.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief Returns the text value for this edit control.
    ///
    /// @return                         Text value of control
    ///
    std::string GetText() const
    {
      std::string text;
      char* ret = m_interface->kodi_gui->control_edit->get_text(m_interface->kodiBase, m_controlHandle);
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          text = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return text;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief Set the cursor position on text.
    ///
    /// @param[in] iPosition            The position to set
    ///
    void SetCursorPosition(unsigned int iPosition)
    {
      m_interface->kodi_gui->control_edit->set_cursor_position(m_interface->kodiBase, m_controlHandle, iPosition);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief To get current cursor position on text field
    ///
    /// @return                         The current cursor position
    ///
    unsigned int GetCursorPosition()
    {
      return m_interface->kodi_gui->control_edit->get_cursor_position(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CEdit
    /// @brief To set field input type which are defined on \ref AddonGUIInputType
    ///
    /// @param[in] type                 The \ref AddonGUIInputType "Add-on input type"
    ///                                 to use
    /// @param[in] heading              The heading text for related keyboard
    ///                                 dialog
    ///
    void SetInputType(AddonGUIInputType type, const std::string& heading)
    {
      m_interface->kodi_gui->control_edit->set_input_type(m_interface->kodiBase, m_controlHandle, static_cast<int>(type), heading.c_str());
    }
    //--------------------------------------------------------------------------
  };

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */
