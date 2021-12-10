/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/gui/controls/edit.h"
#include "../Window.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace controls
{

//==============================================================================
/// @defgroup cpp_kodi_gui_windows_controls_CEdit Control Edit
/// @ingroup cpp_kodi_gui_windows_controls
/// @brief @cpp_class{ kodi::gui::controls::CEdit }
/// **Editable window text control used as an input control for the osd keyboard
/// and other input fields**\n
/// The edit control allows a user to input text in Kodi.
///
/// You can choose the font, size, colour, location and header of the text to be
/// displayed.
///
/// It has the header @ref Edit.h "#include <kodi/gui/controls/Edit.h>"
/// be included to enjoy it.
///
/// Here you find the needed skin partfor a @ref skin_Edit_control "edit control".
///
/// @note The call of the control is only possible from the corresponding
/// window as its class and identification number is required.
///

//==============================================================================
// see gui/definition.h for use of group "cpp_kodi_gui_windows_controls_CEdit_Defs"
///
/// @defgroup cpp_kodi_gui_windows_controls_CEdit_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_gui_windows_controls_CEdit
/// @brief **Library definition values**
///

class ATTR_DLL_LOCAL CEdit : public CAddonGUIControlBase
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief Construct a new control.
  ///
  /// @param[in] window Related window control class
  /// @param[in] controlId Used skin xml control id
  ///
  CEdit(CWindow* window, int controlId) : CAddonGUIControlBase(window)
  {
    m_controlHandle = m_interface->kodi_gui->window->get_control_edit(
        m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
    if (!m_controlHandle)
      kodi::Log(ADDON_LOG_FATAL,
                "kodi::gui::control::CEdit can't create control class from Kodi !!!");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief Destructor.
  ///
  ~CEdit() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief Set the control on window to visible.
  ///
  /// @param[in] visible If true visible, otherwise hidden
  ///
  void SetVisible(bool visible)
  {
    m_interface->kodi_gui->control_edit->set_visible(m_interface->kodiBase, m_controlHandle,
                                                     visible);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief Set's the control's enabled/disabled state.
  ///
  /// @param[in] enabled If true enabled, otherwise disabled
  ///
  void SetEnabled(bool enabled)
  {
    m_interface->kodi_gui->control_edit->set_enabled(m_interface->kodiBase, m_controlHandle,
                                                     enabled);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief To set the text string on edit control.
  ///
  /// @param[in] label Text to show
  ///
  void SetLabel(const std::string& label)
  {
    m_interface->kodi_gui->control_edit->set_label(m_interface->kodiBase, m_controlHandle,
                                                   label.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief Returns the text heading for this edit control.
  ///
  /// @return Heading text
  ///
  std::string GetLabel() const
  {
    std::string label;
    char* ret =
        m_interface->kodi_gui->control_edit->get_label(m_interface->kodiBase, m_controlHandle);
    if (ret != nullptr)
    {
      if (std::strlen(ret))
        label = ret;
      m_interface->free_string(m_interface->kodiBase, ret);
    }
    return label;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief Set's text heading for this edit control.
  ///
  /// @param[in] text                 string or unicode - text string.
  ///
  void SetText(const std::string& text)
  {
    m_interface->kodi_gui->control_edit->set_text(m_interface->kodiBase, m_controlHandle,
                                                  text.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief Returns the text value for this edit control.
  ///
  /// @return Text value of control
  ///
  std::string GetText() const
  {
    std::string text;
    char* ret =
        m_interface->kodi_gui->control_edit->get_text(m_interface->kodiBase, m_controlHandle);
    if (ret != nullptr)
    {
      if (std::strlen(ret))
        text = ret;
      m_interface->free_string(m_interface->kodiBase, ret);
    }
    return text;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief Set the cursor position on text.
  ///
  /// @param[in] position The position to set
  ///
  void SetCursorPosition(unsigned int position)
  {
    m_interface->kodi_gui->control_edit->set_cursor_position(m_interface->kodiBase, m_controlHandle,
                                                             position);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief To get current cursor position on text field.
  ///
  /// @return The current cursor position
  ///
  unsigned int GetCursorPosition()
  {
    return m_interface->kodi_gui->control_edit->get_cursor_position(m_interface->kodiBase,
                                                                    m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CEdit
  /// @brief To set field input type which are defined on @ref AddonGUIInputType.
  ///
  /// @param[in] type The @ref AddonGUIInputType "Add-on input type" to use
  /// @param[in] heading The heading text for related keyboard dialog
  ///
  void SetInputType(AddonGUIInputType type, const std::string& heading)
  {
    m_interface->kodi_gui->control_edit->set_input_type(m_interface->kodiBase, m_controlHandle,
                                                        static_cast<int>(type), heading.c_str());
  }
  //----------------------------------------------------------------------------
};

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
