/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/gui/controls/settings_slider.h"
#include "../Window.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace controls
{

//==============================================================================
/// @defgroup cpp_kodi_gui_windows_controls_CSettingsSlider Control Settings Slider
/// @ingroup cpp_kodi_gui_windows_controls
/// @brief @cpp_class{ kodi::gui::controls::CSettingsSlider }
/// **Window control for moveable slider with text name**\n
/// The settings slider control is used in the settings screens for when an
/// option is best specified on a sliding scale.
///
/// You can choose the position, size, and look of the slider control. It is
/// basically a cross between the button control and a slider control. It has a
/// label and focus and non focus textures, as well as a slider control on the
/// right.
///
/// It has the header @ref SettingsSlider.h "#include <kodi/gui/controls/SettingsSlider.h>"
/// be included to enjoy it.
///
/// Here you find the needed skin part for a @ref Settings_Slider_Control "settings slider control".
///
/// @note The call of the control is only possible from the corresponding
/// window as its class and identification number is required.
///
class ATTR_DLL_LOCAL CSettingsSlider : public CAddonGUIControlBase
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief Construct a new control.
  ///
  /// @param[in] window Related window control class
  /// @param[in] controlId Used skin xml control id
  ///
  CSettingsSlider(CWindow* window, int controlId) : CAddonGUIControlBase(window)
  {
    m_controlHandle = m_interface->kodi_gui->window->get_control_settings_slider(
        m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
    if (!m_controlHandle)
      kodi::Log(ADDON_LOG_FATAL,
                "kodi::gui::controls::CSettingsSlider can't create control class from Kodi !!!");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief Destructor.
  ///
  ~CSettingsSlider() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief Set the control on window to visible.
  ///
  /// @param[in] visible If true visible, otherwise hidden
  ///
  void SetVisible(bool visible)
  {
    m_interface->kodi_gui->control_settings_slider->set_visible(m_interface->kodiBase,
                                                                m_controlHandle, visible);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief Set's the control's enabled/disabled state.
  ///
  /// @param[in] enabled If true enabled, otherwise disabled
  ///
  void SetEnabled(bool enabled)
  {
    m_interface->kodi_gui->control_settings_slider->set_enabled(m_interface->kodiBase,
                                                                m_controlHandle, enabled);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief To set the text string on settings slider.
  ///
  /// @param[in] text Text to show
  ///
  void SetText(const std::string& text)
  {
    m_interface->kodi_gui->control_settings_slider->set_text(m_interface->kodiBase, m_controlHandle,
                                                             text.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief To reset slider on defaults.
  ///
  void Reset()
  {
    m_interface->kodi_gui->control_settings_slider->reset(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief To set the the range as integer of slider, e.g. -10 is the slider
  /// start and e.g. +10 is the from here defined position where it reach the
  /// end.
  ///
  /// Ad default is the range from 0 to 100.
  ///
  /// The integer interval is as default 1 and can be changed with
  /// @ref SetIntInterval.
  ///
  /// @param[in] start Integer start value
  /// @param[in] end Integer end value
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  void SetIntRange(int start, int end)
  {
    m_interface->kodi_gui->control_settings_slider->set_int_range(m_interface->kodiBase,
                                                                  m_controlHandle, start, end);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief Set the slider position with the given integer value. The Range
  /// must be defined with a call from @ref SetIntRange before.
  ///
  /// @param[in] value Position in range to set with integer
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  void SetIntValue(int value)
  {
    m_interface->kodi_gui->control_settings_slider->set_int_value(m_interface->kodiBase,
                                                                  m_controlHandle, value);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief To get the current position as integer value.
  ///
  /// @return The position as integer
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  int GetIntValue() const
  {
    return m_interface->kodi_gui->control_settings_slider->get_int_value(m_interface->kodiBase,
                                                                         m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief To set the interval steps of slider, as default is it 1. If it
  /// becomes changed with this function will a step of the user with the
  /// value fixed here be executed.
  ///
  /// @param[in] interval interval step to set.
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  void SetIntInterval(int interval)
  {
    m_interface->kodi_gui->control_settings_slider->set_int_interval(m_interface->kodiBase,
                                                                     m_controlHandle, interval);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief Sets the percent of the slider.
  ///
  /// @param[in] percent float - Percent value of slide
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  void SetPercentage(float percent)
  {
    m_interface->kodi_gui->control_settings_slider->set_percentage(m_interface->kodiBase,
                                                                   m_controlHandle, percent);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief Returns a float of the percent of the slider.
  ///
  /// @return float - Percent of slider
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  float GetPercentage() const
  {
    return m_interface->kodi_gui->control_settings_slider->get_percentage(m_interface->kodiBase,
                                                                          m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief To set the the range as float of slider, e.g. -25.0 is the slider
  /// start and e.g. +25.0 is the from here defined position where it reach
  /// the end.
  ///
  /// As default is the range 0.0 to 1.0.
  ///
  /// The float interval is as default 0.1 and can be changed with
  /// @ref SetFloatInterval.
  ///
  /// @param[in] start Integer start value
  /// @param[in] end Integer end value
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  void SetFloatRange(float start, float end)
  {
    m_interface->kodi_gui->control_settings_slider->set_float_range(m_interface->kodiBase,
                                                                    m_controlHandle, start, end);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief Set the slider position with the given float value. The Range can
  /// be defined with a call from @ref SetIntRange before, as default it
  /// is 0.0 to 1.0.
  ///
  /// @param[in] value Position in range to set with float
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  void SetFloatValue(float value)
  {
    m_interface->kodi_gui->control_settings_slider->set_float_value(m_interface->kodiBase,
                                                                    m_controlHandle, value);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief To get the current position as float value.
  ///
  /// @return The position as float
  ///
  float GetFloatValue() const
  {
    return m_interface->kodi_gui->control_settings_slider->get_float_value(m_interface->kodiBase,
                                                                           m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSettingsSlider
  /// @brief To set the interval steps of slider, as default is it 0.1 If it
  /// becomes changed with this function will a step of the user with the
  /// value fixed here be executed.
  ///
  /// @param[in] interval interval step to set.
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used.
  ///
  void SetFloatInterval(float interval)
  {
    m_interface->kodi_gui->control_settings_slider->set_float_interval(m_interface->kodiBase,
                                                                       m_controlHandle, interval);
  }
  //----------------------------------------------------------------------------
};

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
