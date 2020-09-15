/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/gui/controls/radio_button.h"
#include "../Window.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace controls
{

//==============================================================================
/// @defgroup cpp_kodi_gui_windows_controls_CRadioButton Control Radio Button
/// @ingroup cpp_kodi_gui_windows_controls
/// @brief @cpp_class{ kodi::gui::controls::CRadioButton }
/// **Window control for a radio button (as used for on/off settings)**\n
/// The radio button control is used for creating push button on/off settings
/// in Kodi.
///
/// You can choose the position, size, and look of the button. When the user
/// clicks on the radio button, the state will change, toggling the extra
/// textures (textureradioon and textureradiooff). Used for settings
/// controls.
///
/// It has the header @ref RadioButton.h "#include <kodi/gui/controls/RadioButton.h>"
/// be included to enjoy it.
///
/// Here you find the needed skin part for a @ref Radio_button_control "radio button control".
///
/// @note The call of the control is only possible from the corresponding
/// window as its class and identification number is required.
///
///
/// --------------------------------------------------------------------------
/// **Example:**
/// ~~~~~~~~~~~~cpp
/// #include <kodi/gui/Window.h>
///
/// #define MY_RADIO_BUTTON_CONTROL 1
///
/// class CMyWindow : public kodi::gui::CWindow
/// {
/// public:
///   CMyWindow()
///
///   void ShowWindow();
///
///   bool OnInit() override;
///   bool OnClick(int controlId) override;
///
/// private:
///   kodi::gui::controls::CSpin m_myRadioButtonControl;
/// };
///
/// CMyWindow::CMyWindow()
///   : kodi::gui::CWindow("my_skin.xml", "skin.estuary", true, false),
///     m_myRadioButtonControl(this, MY_RADIO_BUTTON_CONTROL)
/// {
/// }
///
/// void CMyWindow::ShowWindow()
/// {
///   kodi::gui::CWindow::DoModal();
/// }
///
/// bool CMyWindow::OnInit()
/// {
///   m_myRadioButtonControl.SetSelected(false); // can also on skin set to default
///   return true;
/// }
///
/// bool CMyWindow::OnClick(int controlId)
/// {
///   if (controlId == MY_RADIO_BUTTON_CONTROL)
///   {
///     bool selected = m_myRadioButtonControl.IsSelected();
///     ...
///     }
///     return true;
///   }
///   return false;
/// }
/// ~~~~~~~~~~~~
///
class ATTRIBUTE_HIDDEN CRadioButton : public CAddonGUIControlBase
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRadioButton
  /// @brief Construct a new control.
  ///
  /// @param[in] window Related window control class
  /// @param[in] controlId  Used skin xml control id
  ///
  CRadioButton(CWindow* window, int controlId) : CAddonGUIControlBase(window)
  {
    m_controlHandle = m_interface->kodi_gui->window->get_control_radio_button(
        m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
    if (!m_controlHandle)
      kodi::Log(ADDON_LOG_FATAL,
                "kodi::gui::controls::CRadioButton can't create control class from Kodi !!!");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRadioButton
  /// @brief Destructor.
  ///
  ~CRadioButton() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRadioButton.
  /// @brief Set the control on window to visible.
  ///
  /// @param[in] visible If true visible, otherwise hidden
  ///
  void SetVisible(bool visible)
  {
    m_interface->kodi_gui->control_radio_button->set_visible(m_interface->kodiBase, m_controlHandle,
                                                             visible);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRadioButton
  /// @brief Set's the control's enabled/disabled state.
  ///
  /// @param[in] enabled If true enabled, otherwise disabled
  ///
  void SetEnabled(bool enabled)
  {
    m_interface->kodi_gui->control_radio_button->set_enabled(m_interface->kodiBase, m_controlHandle,
                                                             enabled);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRadioButton
  /// @brief To set the text string on radio button.
  ///
  /// @param[in] label Text to show
  ///
  void SetLabel(const std::string& label)
  {
    m_interface->kodi_gui->control_radio_button->set_label(m_interface->kodiBase, m_controlHandle,
                                                           label.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRadioButton
  /// @brief Get the used text from control.
  ///
  /// @return Text shown
  ///
  std::string GetLabel() const
  {
    std::string label;
    char* ret = m_interface->kodi_gui->control_radio_button->get_label(m_interface->kodiBase,
                                                                       m_controlHandle);
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
  /// @ingroup cpp_kodi_gui_windows_controls_CRadioButton
  /// @brief To set radio button condition to on or off.
  ///
  /// @param[in] selected true set radio button to selection on, otherwise off
  ///
  void SetSelected(bool selected)
  {
    m_interface->kodi_gui->control_radio_button->set_selected(m_interface->kodiBase,
                                                              m_controlHandle, selected);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRadioButton
  /// @brief Get the current selected condition of radio button.
  ///
  /// @return Selected condition
  ///
  bool IsSelected() const
  {
    return m_interface->kodi_gui->control_radio_button->is_selected(m_interface->kodiBase,
                                                                    m_controlHandle);
  }
  //----------------------------------------------------------------------------
};

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
