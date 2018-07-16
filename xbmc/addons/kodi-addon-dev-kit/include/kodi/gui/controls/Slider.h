/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
  /// \defgroup cpp_kodi_gui_controls_CSlider Control Slider
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::controls::CSlider }
  /// **Window control for moveable slider**
  ///
  /// The slider control is used for things where a sliding bar best  represents
  /// the operation at hand (such as a volume control or seek control).  You can
  /// choose the position, size, and look of the slider control.
  ///
  /// It has the header \ref Slider.h "#include <kodi/gui/controls/Slider.h>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref Slider_Control "slider control"
  ///
  /// @note The  call of the  control  is only  possible from  the corresponding
  /// window as its class and identification number is required.
  ///
  class CSlider : public CAddonGUIControlBase
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CSlider(CWindow* window, int controlId)
      : CAddonGUIControlBase(window)
    {
      m_controlHandle = m_interface->kodi_gui->window->get_control_slider(m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
      if (!m_controlHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::controls::CSlider can't create control class from Kodi !!!");
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief Destructor
    ///
    ~CSlider() override = default;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible              If true visible, otherwise hidden
    ///
    void SetVisible(bool visible)
    {
      m_interface->kodi_gui->control_slider->set_visible(m_interface->kodiBase, m_controlHandle, visible);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief Set's the control's enabled/disabled state
    ///
    /// @param[in] enabled              If true enabled, otherwise disabled
    ///
    void SetEnabled(bool enabled)
    {
      m_interface->kodi_gui->control_slider->set_enabled(m_interface->kodiBase, m_controlHandle, enabled);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief To reset slider on defaults
    ///
    void Reset()
    {
      m_interface->kodi_gui->control_slider->reset(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief With GetDescription becomes a string value of position returned.
    ///
    /// @return                        Text string about current slider position
    ///
    /// The following are the text definition returned from this:
    /// | Value     | Without range selection | With range selection           |
    /// |:---------:|:------------------------|:-------------------------------|
    /// | float     | <c>%2.2f</c>            | <c>[%2.2f, %2.2f]</c>          |
    /// | integer   | <c>%i</c>               | <c>[%i, %i]</c>                |
    /// | percent   | <c>%i%%</c>             | <c>[%i%%, %i%%]</c>            |
    ///
    std::string GetDescription() const
    {
      std::string text;
      char* ret = m_interface->kodi_gui->control_slider->get_description(m_interface->kodiBase, m_controlHandle);
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
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief To set the the range as integer of slider, e.g. -10 is the slider
    /// start and e.g. +10 is the from here defined position where it  reach the
    /// end.
    ///
    /// Ad default is the range from 0 to 100.
    ///
    /// The integer interval is as default 1 and can be changed with
    /// @ref SetIntInterval.
    ///
    /// @param[in] start                Integer start value
    /// @param[in] end                  Integer end value
    ///
    /// @note Percent, floating point or  integer are alone possible.  Combining
    /// these different values can be not together and can, therefore,  only one
    /// each can be used.
    ///
    void SetIntRange(int start, int end)
    {
      m_interface->kodi_gui->control_slider->set_int_range(m_interface->kodiBase, m_controlHandle, start, end);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup CSlider
    /// @brief Set the slider position  with the given integer value.  The Range
    /// must be defined with a call from \ref SetIntRange before.
    ///
    /// @param[in] value                Position in range to set with integer
    ///
    /// @note Percent, floating point or integer  are alone possible.  Combining
    /// these different values can be not together and can, therefore,  only one
    /// each can be used.
    ///
    void SetIntValue(int value)
    {
      m_interface->kodi_gui->control_slider->set_int_value(m_interface->kodiBase, m_controlHandle, value);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief To get the current position as integer value.
    ///
    /// @return                         The position as integer
    ///
    /// @note Percent,  floating point or integer are alone possible.  Combining
    /// these  different  values  can be not together and can,  therefore,  only
    /// one each can be used.
    ///
    int GetIntValue() const
    {
      return m_interface->kodi_gui->control_slider->get_int_value(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief To set the interval  steps of slider,  as default is it 1.  If it
    /// becomes  changed  with this function  will a step  of the  user with the
    /// value fixed here be executed.
    ///
    /// @param[in] interval             Intervall step to set.
    ///
    /// @note Percent,  floating point or integer are alone possible.  Combining
    /// these different values can be not together and can, therefore,  only one
    /// each can be used.
    ///
    void SetIntInterval(int interval)
    {
      m_interface->kodi_gui->control_slider->set_int_interval(m_interface->kodiBase, m_controlHandle, interval);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief Sets the percent of the slider.
    ///
    /// @param[in] percent              float - Percent value of slide
    ///
    /// @note Percent, floating point or  integer are alone possible.  Combining
    /// these different values can be not together and can, therefore,  only one
    /// each can be used.
    ///
    void SetPercentage(float percent)
    {
      m_interface->kodi_gui->control_slider->set_percentage(m_interface->kodiBase, m_controlHandle, percent);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief Returns a float of the percent of the slider.
    ///
    /// @return                         float - Percent of slider
    ///
    /// @note Percent, floating point or integer  are alone possible.  Combining
    /// these different values can be not together and can, therefore,  only one
    /// each can be used.
    ///
    float GetPercentage() const
    {
      return m_interface->kodi_gui->control_slider->get_percentage(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief To set the the range as float of slider, e.g. -25.0 is the slider
    /// start and e.g. +25.0 is  the from here  defined position  where it reach
    /// the end.
    ///
    /// As default is the range 0.0 to 1.0.
    ///
    /// The float interval is as default 0.1 and can be changed with
    /// @ref SetFloatInterval.
    ///
    /// @param[in] start                Integer start value
    /// @param[in] end                  Integer end value
    ///
    /// @note Percent,  floating point or integer are alone possible.  Combining
    /// these  different  values  can be not together and can,  therefore,  only
    /// one each can be used.
    ///
    void SetFloatRange(float start, float end)
    {
      m_interface->kodi_gui->control_slider->set_float_range(m_interface->kodiBase, m_controlHandle, start, end);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief Set the slider  position with the  given float value.  The  Range
    /// can be defined with a call from  \ref SetIntRange before,  as default it
    /// is 0.0 to 1.0.
    ///
    /// @param[in] value                Position in range to set with float
    ///
    /// @note Percent, floating  point or integer are alone possible.  Combining
    /// these different values can be not together and can, therefore,  only one
    /// each can be used.
    ///
    void SetFloatValue(float value)
    {
      m_interface->kodi_gui->control_slider->set_float_value(m_interface->kodiBase, m_controlHandle, value);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief To get the current position as float value.
    ///
    /// @return                         The position as float
    ///
    float GetFloatValue() const
    {
      return m_interface->kodi_gui->control_slider->get_float_value(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CSlider
    /// @brief To set the interval  steps of slider, as default is it 0.1  If it
    /// becomes changed  with this  function  will a step  of the user  with the
    /// value fixed here be executed.
    ///
    /// @param[in] interval             Intervall step to set.
    ///
    /// @note Percent, floating point or integer  are alone possible.  Combining
    /// these different  values can be  not together  and can,  therefore,  only
    /// one each can be used.
    ///
    void SetFloatInterval(float interval)
    {
      m_interface->kodi_gui->control_slider->set_float_interval(m_interface->kodiBase, m_controlHandle, interval);
    }
    //--------------------------------------------------------------------------
  };

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */
