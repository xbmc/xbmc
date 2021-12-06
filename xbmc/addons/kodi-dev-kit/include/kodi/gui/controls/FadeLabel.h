/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/gui/controls/fade_label.h"
#include "../Window.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace controls
{

//==============================================================================
/// @defgroup cpp_kodi_gui_windows_controls_CFadeLabel Control Fade Label
/// @ingroup cpp_kodi_gui_windows_controls
/// @brief @cpp_class{ kodi::gui::controls::CFadeLabel }
/// **Window control used to show multiple pieces of text in the same position,
/// by fading from one to the other**\n
/// The fade label control is used for displaying multiple pieces of text in
/// the same space in Kodi.
///
/// You can choose the font, size, colour, location and contents of the text to
/// be displayed. The first piece of information to display fades in over 50
/// frames, then scrolls off to the left. Once it is finished scrolling off
/// screen, the second piece of information fades in and the process repeats.
/// A fade label control is not supported in a list container.
///
/// It has the header @ref FadeLabel.h "#include <kodi/gui/controls/FadeLabel.h>"
/// be included to enjoy it.
///
/// Here you find the needed skin part for a @ref Fade_Label_Control "fade label control".
///
/// @note The call of the control is only possible from the corresponding
/// window as its class and identification number is required.
///
class ATTR_DLL_LOCAL CFadeLabel : public CAddonGUIControlBase
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CFadeLabel
  /// @brief Construct a new control.
  ///
  /// @param[in] window Related window control class
  /// @param[in] controlId Used skin xml control id
  ///
  CFadeLabel(CWindow* window, int controlId) : CAddonGUIControlBase(window)
  {
    m_controlHandle = m_interface->kodi_gui->window->get_control_fade_label(
        m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
    if (!m_controlHandle)
      kodi::Log(ADDON_LOG_FATAL,
                "kodi::gui::controls::CFadeLabel can't create control class from Kodi !!!");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CFadeLabel
  /// @brief Destructor.
  ///
  ~CFadeLabel() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CFadeLabel
  /// @brief Set the control on window to visible.
  ///
  /// @param[in] visible If true visible, otherwise hidden
  ///
  void SetVisible(bool visible)
  {
    m_interface->kodi_gui->control_fade_label->set_visible(m_interface->kodiBase, m_controlHandle,
                                                           visible);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CFadeLabel
  /// @brief To add additional text string on fade label.
  ///
  /// @param[in] label Text to show
  ///
  void AddLabel(const std::string& label)
  {
    m_interface->kodi_gui->control_fade_label->add_label(m_interface->kodiBase, m_controlHandle,
                                                         label.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CFadeLabel
  /// @brief Get the used text from button.
  ///
  /// @return Text shown
  ///
  std::string GetLabel() const
  {
    std::string label;
    char* ret = m_interface->kodi_gui->control_fade_label->get_label(m_interface->kodiBase,
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
  /// @ingroup cpp_kodi_gui_windows_controls_CFadeLabel
  /// @brief To enable or disable scrolling on fade label.
  ///
  /// @param[in] scroll To enable scrolling set to true, otherwise is disabled
  ///
  void SetScrolling(bool scroll)
  {
    m_interface->kodi_gui->control_fade_label->set_scrolling(m_interface->kodiBase, m_controlHandle,
                                                             scroll);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CFadeLabel
  /// @brief To reset al inserted labels.
  ///
  void Reset()
  {
    m_interface->kodi_gui->control_fade_label->reset(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------
};

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
