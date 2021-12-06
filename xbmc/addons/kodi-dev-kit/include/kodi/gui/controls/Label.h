/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/gui/controls/label.h"
#include "../Window.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace controls
{

//==============================================================================
/// @defgroup cpp_kodi_gui_windows_controls_CLabel Control Label
/// @ingroup cpp_kodi_gui_windows_controls
/// @brief @cpp_class{ kodi::gui::controls::CLabel }
/// **Window control used to show some lines of text**\n
/// The label control is used for displaying text in Kodi. You can choose
/// the font, size, colour, location and contents of the text to be displayed.
///
/// It has the header @ref Label.h "#include <kodi/gui/controls/Label.h>"
/// be included to enjoy it.
///
/// Here you find the needed skin part for a @ref Label_Control "label control".
///
/// @note The call of the control is only possible from the corresponding
/// window as its class and identification number is required.
///
class ATTR_DLL_LOCAL CLabel : public CAddonGUIControlBase
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CLabel
  /// @brief Construct a new control.
  ///
  /// @param[in] window Related window control class
  /// @param[in] controlId Used skin xml control id
  ///
  CLabel(CWindow* window, int controlId) : CAddonGUIControlBase(window)
  {
    m_controlHandle = m_interface->kodi_gui->window->get_control_label(
        m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
    if (!m_controlHandle)
      kodi::Log(ADDON_LOG_FATAL,
                "kodi::gui::controls::CLabel can't create control class from Kodi !!!");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CLabel
  /// @brief Destructor.
  ///
  ~CLabel() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CLabel
  /// @brief Set the control on window to visible.
  ///
  /// @param[in] visible If true visible, otherwise hidden
  ///
  void SetVisible(bool visible)
  {
    m_interface->kodi_gui->control_label->set_visible(m_interface->kodiBase, m_controlHandle,
                                                      visible);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CLabel
  /// @brief To set the text string on label.
  ///
  /// @param[in] text Text to show
  ///
  void SetLabel(const std::string& text)
  {
    m_interface->kodi_gui->control_label->set_label(m_interface->kodiBase, m_controlHandle,
                                                    text.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CLabel
  /// @brief Get the used text from control.
  ///
  /// @return Used text on label control
  ///
  std::string GetLabel() const
  {
    std::string label;
    char* ret =
        m_interface->kodi_gui->control_label->get_label(m_interface->kodiBase, m_controlHandle);
    if (ret != nullptr)
    {
      if (std::strlen(ret))
        label = ret;
      m_interface->free_string(m_interface->kodiBase, ret);
    }
    return label;
  }
  //----------------------------------------------------------------------------
};

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
