/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/gui/controls/spin.h"
#include "../Window.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace controls
{

//==============================================================================
/// @defgroup cpp_kodi_gui_windows_controls_CSpin Control Spin
/// @ingroup cpp_kodi_gui_windows_controls
/// @brief @cpp_class{ kodi::gui::controls::CSpin }
/// **Window control used for cycling up/down controls**\n
/// The settings spin control is used in the settings screens for when a list
/// of options can be chosen from using up/down arrows.
///
/// You can choose the position, size, and look of the spin control. It is
/// basically a cross between the button control and a spin control. It has a
/// label and focus and non focus textures, as well as a spin control on the
/// right.
///
/// It has the header @ref Spin.h "#include <kodi/gui/controls/Spin.h>"
/// be included to enjoy it.
///
/// Here you find the needed skin part for a @ref Spin_Control "spin control".
///
/// @note The call of the control is only possible from the corresponding
/// window as its class and identification number is required.
///
/// --------------------------------------------------------------------------
/// **Example:**
/// ~~~~~~~~~~~~cpp
/// #include <kodi/gui/Window.h>
///
/// #define MY_SPIN_CONTROL 1
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
///   kodi::gui::controls::CSpin m_mySpinControl;
/// };
///
/// CMyWindow::CMyWindow()
///   : kodi::gui::CWindow("my_skin.xml", "skin.estuary", true, false),
///     m_mySpinControl(this, MY_SPIN_CONTROL)
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
///   m_mySpinControl.SetType(kodi::gui::controls::ADDON_SPIN_CONTROL_TYPE_INT);
///   m_mySpinControl.SetIntRange(1, 80);
///   return true;
/// }
///
/// bool CMyWindow::OnClick(int controlId)
/// {
///   if (controlId == MY_SPIN_CONTROL)
///   {
///     int value = m_mySpinControl.GetIntValue();
///     ...
///     }
///     return true;
///   }
///   return false;
/// }
/// ~~~~~~~~~~~~
///

//==============================================================================
/// @ingroup cpp_kodi_gui_windows_controls_CSpin
/// @anchor AddonGUISpinControlType
/// @brief The values here defines the used value format for steps on
/// spin control.
///
typedef enum AddonGUISpinControlType
{
  /// One spin step interpreted as integer
  ADDON_SPIN_CONTROL_TYPE_INT = 1,
  /// One spin step interpreted as floating point value
  ADDON_SPIN_CONTROL_TYPE_FLOAT = 2,
  /// One spin step interpreted as text string
  ADDON_SPIN_CONTROL_TYPE_TEXT = 3,
  /// One spin step interpreted as a page change value
  ADDON_SPIN_CONTROL_TYPE_PAGE = 4
} AddonGUISpinControlType;
//------------------------------------------------------------------------------

class ATTR_DLL_LOCAL CSpin : public CAddonGUIControlBase
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief Construct a new control.
  ///
  /// @param[in] window Related window control class
  /// @param[in] controlId Used skin xml control id
  ///
  CSpin(CWindow* window, int controlId) : CAddonGUIControlBase(window)
  {
    m_controlHandle = m_interface->kodi_gui->window->get_control_spin(
        m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
    if (!m_controlHandle)
      kodi::Log(ADDON_LOG_FATAL,
                "kodi::gui::controls::CSpin can't create control class from Kodi !!!");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief Destructor.
  ///
  ~CSpin() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief Set the control on window to visible.
  ///
  /// @param[in] visible If true visible, otherwise hidden
  ///
  void SetVisible(bool visible)
  {
    m_interface->kodi_gui->control_spin->set_visible(m_interface->kodiBase, m_controlHandle,
                                                     visible);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief Set's the control's enabled/disabled state.
  ///
  /// @param[in] enabled If true enabled, otherwise disabled
  ///
  void SetEnabled(bool enabled)
  {
    m_interface->kodi_gui->control_spin->set_enabled(m_interface->kodiBase, m_controlHandle,
                                                     enabled);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To set the text string on spin control.
  ///
  /// @param[in] text Text to show as name for spin
  ///
  void SetText(const std::string& text)
  {
    m_interface->kodi_gui->control_spin->set_text(m_interface->kodiBase, m_controlHandle,
                                                  text.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To reset spin control to defaults.
  ///
  void Reset()
  {
    m_interface->kodi_gui->control_spin->reset(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To set the with SpinControlType defined types of spin.
  ///
  /// @param[in] type The type to use
  ///
  /// @note See description of @ref AddonGUISpinControlType for available types.
  ///
  void SetType(AddonGUISpinControlType type)
  {
    m_interface->kodi_gui->control_spin->set_type(m_interface->kodiBase, m_controlHandle,
                                                  (int)type);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To add a label entry in spin defined with a value as string.
  ///
  /// Format must be set to @ref ADDON_SPIN_CONTROL_TYPE_TEXT to use this function.
  ///
  /// @param[in] label Label string to view on skin
  /// @param[in] value String value to use for selection of them
  ///
  void AddLabel(const std::string& label, const std::string& value)
  {
    m_interface->kodi_gui->control_spin->add_string_label(m_interface->kodiBase, m_controlHandle,
                                                          label.c_str(), value.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To add a label entry in spin defined with a value as integer.
  ///
  /// Format must be set to @ref ADDON_SPIN_CONTROL_TYPE_INT to use this function.
  ///
  /// @param[in] label Label string to view on skin
  /// @param[in] value Integer value to use for selection of them.
  ///
  void AddLabel(const std::string& label, int value)
  {
    m_interface->kodi_gui->control_spin->add_int_label(m_interface->kodiBase, m_controlHandle,
                                                       label.c_str(), value);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To change the spin to position with them string as value.
  ///
  /// Format must be set to @ref ADDON_SPIN_CONTROL_TYPE_TEXT to use this function.
  ///
  /// @param[in] value String value to change to
  ///
  void SetStringValue(const std::string& value)
  {
    m_interface->kodi_gui->control_spin->set_string_value(m_interface->kodiBase, m_controlHandle,
                                                          value.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To get the current spin control position with text string value.
  ///
  /// Format must be set to @ref ADDON_SPIN_CONTROL_TYPE_TEXT to use this function.
  ///
  /// @return Currently selected string value
  ///
  std::string GetStringValue() const
  {
    std::string value;
    char* ret = m_interface->kodi_gui->control_spin->get_string_value(m_interface->kodiBase,
                                                                      m_controlHandle);
    if (ret != nullptr)
    {
      if (std::strlen(ret))
        value = ret;
      m_interface->free_string(m_interface->kodiBase, ret);
    }
    return value;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To set the the range as integer of slider, e.g. -10 is the slider
  /// start and e.g. +10 is the from here defined position where it reach the
  /// end.
  ///
  /// Ad default is the range from 0 to 100.
  ///
  /// @param[in] start Integer start value
  /// @param[in] end Integer end value
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used and must be defined with @ref SetType before.
  ///
  void SetIntRange(int start, int end)
  {
    m_interface->kodi_gui->control_spin->set_int_range(m_interface->kodiBase, m_controlHandle,
                                                       start, end);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief Set the slider position with the given integer value. The Range
  /// must be defined with a call from @ref SetIntRange before.
  ///
  /// @param[in] value Position in range to set with integer
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used and must be defined with @ref SetType before.
  ///
  void SetIntValue(int value)
  {
    m_interface->kodi_gui->control_spin->set_int_value(m_interface->kodiBase, m_controlHandle,
                                                       value);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To get the current position as integer value.
  ///
  /// @return The position as integer
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used and must be defined with @ref SetType before.
  ///
  int GetIntValue() const
  {
    return m_interface->kodi_gui->control_spin->get_int_value(m_interface->kodiBase,
                                                              m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To set the the range as float of spin, e.g. -25.0 is the spin
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
  /// one each can be used and must be defined with @ref SetType before.
  ///
  void SetFloatRange(float start, float end)
  {
    m_interface->kodi_gui->control_spin->set_float_range(m_interface->kodiBase, m_controlHandle,
                                                         start, end);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief Set the spin position with the given float value. The Range
  /// can be defined with a call from @ref SetIntRange before, as default it
  /// is 0.0 to 1.0.
  ///
  /// @param[in] value Position in range to set with float
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used and must be defined with @ref SetType before.
  ///
  void SetFloatValue(float value)
  {
    m_interface->kodi_gui->control_spin->set_float_value(m_interface->kodiBase, m_controlHandle,
                                                         value);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To get the current position as float value.
  ///
  /// @return The position as float
  ///
  float GetFloatValue() const
  {
    return m_interface->kodi_gui->control_spin->get_float_value(m_interface->kodiBase,
                                                                m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CSpin
  /// @brief To set the interval steps of spin, as default is it 0.1 If it
  /// becomes changed with this function will a step of the user with the
  /// value fixed here be executed.
  ///
  /// @param[in] interval Intervall step to set.
  ///
  /// @note Percent, floating point or integer are alone possible. Combining
  /// these different values can be not together and can, therefore, only
  /// one each can be used and must be defined with @ref SetType before.
  ///
  void SetFloatInterval(float interval)
  {
    m_interface->kodi_gui->control_spin->set_float_interval(m_interface->kodiBase, m_controlHandle,
                                                            interval);
  }
  //----------------------------------------------------------------------------
};

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
